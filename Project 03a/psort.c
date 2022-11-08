#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define MAP_HUGE_1GB (30 << MAP_HUGE_SHIFT)

// Global Variables
int THREAD_MAX = 0;
int THREAD_USED = 0;
pthread_mutex_t merge_mutex;
int multi_on;
char* buf;
int bufSize;

// Compares two records and returns 1 if the first record is greater than the second record
int compareRecords(char* record, char* records) { 
    // Variable initializations
    int key = *(int*) record;
    int keys = *(int*) records;
    
    if(key <= keys) { // If the first record is less than or equal to the second record
        return -1;
    }else { // If the first record is greater than the second record
        return 1;
    }

    return 0;
}

// Returns the stride of the index
int stride(int index) {
    return index * 100;
}

// Merges two sorted arrays
void merge(int left, int mid, int right) {
    // Variable declarations
    int i = 0;
    int j = 0;
    int k = left;
    int n1 = mid - left + 1;
    int n2 = right - mid;

    // Array declarations
    char* Left = malloc((n1 * 100) * sizeof(char));
    char* Right = malloc((n2 * 100) * sizeof(char));

    // Copy array 
    memcpy(Left, (buf + stride(left)), n1 * 100);
    memcpy(Right, (buf + stride(mid + 1)), n2 * 100);

    while(i < n1 && j < n2) { // While the left and right arrays have not been fully traversed
        if(compareRecords((Left + stride(i)), (Right + stride(j))) < 0) { // If the left array is less than the right array
            memcpy((buf + stride(k)), &Left[stride(i)], 100);
            i++;
        } else { // If the right array is less than the left array
            memcpy((buf + stride(k)), &Right[stride(j)], 100);
            j++;
        }
        k++;
    }
    while(i < n1) { // While the left array has not been fully traversed
        memcpy((buf + stride(k)), &Left[stride(i)], 100);
        i++;
        k++;
    }
    while(j < n2) { // While the right array has not been fully traversed
        memcpy((buf + stride(k)), &Right[stride(j)], 100);
        j++;
        k++;
    }

    free(Left);
    free(Right);
}

// Merge sort to be used by threads
void* mergeSort(void* arg) {
    // Variable initializations 
    int *args = (int*) arg;
    int left = args[0];
    int right = args[1];
    int mid = left + (right - left) / 2;
    int args1[2];
    int args2[2];
    int flag_1 = 0;
    int flag_2 = 0;

    // Array positions
    args1[0] = left;
    args1[1] = mid;
    args2[0] = mid+1;
    args2[1] = right;

    if(left < right) { // If the left index is less than the right index
        pthread_t new_thread_1, new_thread_2; // Create new threads
        if(THREAD_USED < THREAD_MAX && multi_on == 1) { // If the number of threads used is less than the maximum number of threads and multi-threading is on
            THREAD_USED++;
            //printf("Thread num: %d\n",THREAD_USED);
            flag_1 = 1;
            pthread_create(&new_thread_1, NULL, mergeSort, args1);
            //printf("Thread num: %d\n",THREAD_USED);
        } else { // If multi-threading is off
            mergeSort(args1);
            //printf("Thread num: MAX\n");
        }
        if(THREAD_USED < THREAD_MAX && multi_on == 1) { // If the number of threads used is less than the maximum number of threads and multi-threading is on
            THREAD_USED++;
            //printf("Thread num: %d\n",THREAD_USED);
            flag_2 = 1;
            pthread_create(&new_thread_2, NULL, mergeSort, args2);
            //printf("Thread num: %d\n",THREAD_USED);
        } else { // If multi-threading is off
            mergeSort(args2);
            //printf("Thread num: MAX\n");
        }
        if(flag_1 == 1) { // If the first thread was created
            pthread_join(new_thread_1, NULL);
            THREAD_USED--;
            flag_1 = 0;
        }
        if(flag_2 == 1) { // If the second thread was created
            pthread_join(new_thread_2, NULL);
            THREAD_USED--;
            flag_2 = 0;
        }
        
        pthread_mutex_lock(&merge_mutex); // Lock the mutex
        merge(left, mid, right); // Merge the two arrays
        pthread_mutex_unlock(&merge_mutex); // Unlock the mutex
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

    buf = mmap(NULL, bufSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_NORESERVE, fd, 0); // Map the file to memory
    int numRecords = bufSize / 100; // Get the number of records
    int arg[2]; // Array declaration

    if(numRecords < 20000) { // If the number of records is less than 20000
        multi_on = 0;
    } else { // If the number of records is greater than 20000
        multi_on = 1;
    }

    THREAD_MAX = get_nprocs(); // Set max thread to number of processors
    //THREAD_MAX = 8;
    THREAD_USED = 1; // Set used thread to be 1

    // Array declarations
    arg[0] = 0;
    arg[1] = numRecords - 1;

    pthread_t pthread_main; // Create new thread
    pthread_create(&pthread_main, NULL, mergeSort, arg); // Create new thread
    pthread_join(pthread_main, NULL); // Join the thread

    //printf("Final_Thread num: %d\n",THREAD_USED);

    // Variable declarations
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

    fsync(file); // Sync the file
    close(file); // Close the file

    return 0;
}
