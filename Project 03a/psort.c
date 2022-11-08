#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define MAP_HUGE_1GB (30 << MAP_HUGE_SHIFT) // 1GB

// Global Variables
int THREAD_MAX = 0;
int THREAD_USED = 0;
int multi_on;
char* buf;
int bufSize;

// This function is used to compare two records
int compareRecords(char* record, char* records) {
    int key = *(int*) record; // Get the first 4 bytes of the first record
    int keys = *(int*) records; // Get the first 4 bytes of the second record
    
    if(key <= keys) { // If the first record is smaller than the second record
        return -1;
    } else { // If the first record is larger than the second record
        return 1;
    }
}

// This function is used to calculate the offset of a record
int stride(int index) { 
    return index * 100;
}

// Merge two sorted arrays
void merge(int left, int mid, int right) {
    // Variable initializations
    int i = 0;
    int j = 0;
    int k = left;
    int n1 = mid - left + 1;
    int n2 = right - mid;

    char* Left = malloc((n1 * 100) * sizeof(char)); // Allocate memory for the first array
    char* Right = malloc((n2 * 100) * sizeof(char)); // Allocate memory for the second array

    memcpy(Left, (buf + stride(left)), n1 * 100); // Copy the first array into the allocated memory
    memcpy(Right, (buf + stride(mid + 1)), n2 * 100); // Copy the second array into the allocated memory

    while(i < n1 && j < n2) { // While there are still elements in both arrays
        if(compareRecords((Left + stride(i)), (Right + stride(j))) < 0) {
            memcpy((buf + stride(k)), &Left[stride(i)], 100);
            i++;
        } else { // If the first record is larger than the second record
            memcpy((buf + stride(k)), &Right[stride(j)], 100);
            j++;
        }
        k++;
    }

    while(i < n1) { // Copy the remaining elements of Left[], if there are any
        memcpy((buf + stride(k)), &Left[stride(i)], 100);
        i++;
        k++;
    }

    free(Left); // Free cache

    while(j < n2) { // Copy the remaining elements of Right[], if there are any
        memcpy((buf + stride(k)), &Right[stride(j)], 100);
        j++;
        k++;
    }

    free(Right); // Free cache

}

// Merge sort function
void* mergeSort(void* arg) {
    // Variable initializations
    int *args = (int*) arg;
    int left = args[0];
    int right = args[1];
    int mid = left + (right - left) / 2;
    int args1[2];
    int args2[2];

    // Initialization array positions
    args1[0] = left;
    args1[1] = mid;
    args2[0] = mid + 1;
    args2[1] = right;

    if(left < right) { // If the left index is smaller than the right index
        pthread_t new_thread_1, new_thread_2;
        if(THREAD_USED < THREAD_MAX) { // If there are still threads available
            THREAD_USED++;
            //printf("Thread num: %d\n",THREAD_USED);
            pthread_create(&new_thread_1, NULL, mergeSort, args1);
            pthread_join(new_thread_1, NULL);
            THREAD_USED--;
            //printf("Thread num: %d\n",THREAD_USED);
        } else { // If there are no threads available
            mergeSort(args1);
            //printf("Thread num: MAX\n");
        }
        if(THREAD_USED < THREAD_MAX) { // If there are still threads available
            THREAD_USED++;
            //printf("Thread num: %d\n",THREAD_USED);
            pthread_create(&new_thread_2, NULL, mergeSort, args2);
            pthread_join(new_thread_2, NULL);
            THREAD_USED--;
            //printf("Thread num: %d\n",THREAD_USED);
        } else { // If there are no threads available
            mergeSort(args2);
            //printf("Thread num: MAX\n");
        }

        merge(left, mid, right); // Merge the two sorted arrays
    }

    return NULL;
}

// Main function
int main(int argc, char *argv[]) {
    struct stat st; // File status
    int fd = open(argv[1], O_RDONLY, S_IRUSR | S_IWUSR); // Open the file

    if (fstat(fd, &st) == -1) { // If the file cannot be opened
        fprintf(stderr, "An error has occurred\n");
        exit(0);
    }
    
    bufSize = st.st_size; // Get the size of the file

    if (bufSize == 0) { // If the file is empty
        fprintf(stderr, "An error has occurred\n");
        exit(0);
    }

    buf = mmap(NULL, bufSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_NORESERVE, fd, 0); // Map the file into memory
    int numRecords = bufSize / 100; // Get the number of records in the file
    int arg[2]; // Argument array

    if(numRecords < 20000) { // If the number of records is less than 20000
        multi_on = 0;
    } else { // If the number of records is greater than 20000
        multi_on = 1;
    }
    
    THREAD_MAX = get_nprocs(); // Get the number of cores
    THREAD_USED = 1; // Set the number of threads used to 1
    arg[0] = 0; // Set the left index to 0
    arg[1] = numRecords-1; // Set the right index to the number of records - 1
    pthread_t pthread_main; // Create a new thread

    pthread_create(&pthread_main, NULL, mergeSort, arg); // Create a new thread
    pthread_join(pthread_main, NULL); // Wait for the thread to finish

    //printf("Final_Thread num: %d\n",THREAD_USED);
    // Variable initializations
    int file;
    int ret;

    file = creat(argv[2], S_IWUSR | S_IRUSR); // Create the output file

    if(file < -1) { // If the file cannot be created
        perror("creat() error\n");
        exit(1);
    }

    ret = write(file, buf, bufSize); // Write the sorted records to the output file

    if(ret < -1) { // If the file cannot be written to
        perror("write() error\n");
        exit(1);
    }

    fsync(file); // Flush the file
    close(file); // Close the file

    return 0;
}