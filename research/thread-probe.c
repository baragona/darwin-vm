/* Sample guest threads with bounded frame-pointer walks.
 * Compile with Darwin SDK headers, link matching iOS restore libSystem.
 * The low-39-bit frame-address mask is an experimental PAC-stripping heuristic;
 * reported return addresses are candidates, not a validated unwinder.
 */
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach/arm/thread_status.h>
#include <mach-o/dyld_images.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
static uint64_t address(uint64_t value) { return value & UINT64_C(0x7fffffffff); }
int sample_task_threads(task_t task) {
    kern_return_t kr;
    thread_t sampler = mach_thread_self();
    struct task_dyld_info info = {0};
    mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;
    if (task_info(task, TASK_DYLD_INFO, (task_info_t)&info, &count) == KERN_SUCCESS) {
        struct dyld_all_image_infos images = {0};
        mach_vm_size_t actual = 0;
        kr = mach_vm_read_overwrite(task, info.all_image_info_addr, sizeof(images),
                                   (mach_vm_address_t)&images, &actual);
        if (kr == KERN_SUCCESS) printf("DYLD_SLIDE=0x%llx CACHE_BASE=0x%llx\n",
            (unsigned long long)images.sharedCacheSlide,
            (unsigned long long)images.sharedCacheBaseAddress);
    }
    thread_act_array_t threads = 0;
    mach_msg_type_number_t number = 0;
    kr = task_threads(task, &threads, &number);
    printf("THREADS_RESULT=%d COUNT=%u\n", kr, number);
    if (kr == KERN_SUCCESS) for (unsigned i = 0; i < number; ++i) {
        if (threads[i] == sampler) continue;
        kr = thread_suspend(threads[i]);
        if (kr != KERN_SUCCESS) { printf("THREAD %u SUSPEND_RESULT=%d\n", i, kr); continue; }
        arm_thread_state64_t state = {0};
        count = ARM_THREAD_STATE64_COUNT;
        kr = thread_get_state(threads[i], ARM_THREAD_STATE64, (thread_state_t)&state, &count);
        if (kr != KERN_SUCCESS) { thread_resume(threads[i]); printf("THREAD %u RESULT=%d\n", i, kr); continue; }
        uint64_t fp = address(arm_thread_state64_get_fp(state));
        uint64_t original_fp = fp, frames[24];
        unsigned depths = 0;
        for (; fp && depths < 24; ++depths) {
            uint64_t frame[2]; mach_vm_size_t actual = 0;
            kr = mach_vm_read_overwrite(task, fp, sizeof(frame), (mach_vm_address_t)frame, &actual);
            if (kr != KERN_SUCCESS || actual != sizeof(frame)) break;
            frames[depths] = address(frame[1]);
            uint64_t next = address(frame[0]);
            if (next <= fp || next - fp > 1024 * 1024 || next % 16) { ++depths; break; }
            fp = next;
        }
        /* Resume before stdio: a suspended same-process thread may hold its locks. */
        kr = thread_resume(threads[i]);
        printf("THREAD %u PC=0x%llx LR=0x%llx FP=0x%llx\n", i,
            (unsigned long long)address(arm_thread_state64_get_pc(state)),
            (unsigned long long)address(arm_thread_state64_get_lr(state)),
            (unsigned long long)original_fp);
        for (unsigned depth = 0; depth < depths; ++depth)
            printf(" FRAME %u RETURN=0x%llx\n", depth, (unsigned long long)frames[depth]);
        printf("THREAD %u RESUME_RESULT=%d\n", i, kr);
    }
    if (threads) {
        for (unsigned i = 0; i < number; ++i) mach_port_deallocate(mach_task_self(), threads[i]);
        vm_deallocate(mach_task_self(), (vm_address_t)threads, number * sizeof(thread_t));
    }
    mach_port_deallocate(mach_task_self(), sampler);
    fflush(stdout);
    return 0;
}
#ifndef THREAD_PROBE_LIBRARY
int main(int argc, char **argv) {
    if (argc != 2) { printf("usage: thread-probe PID\n"); return 2; }
    int pid = atoi(argv[1]);
    if (pid <= 1) { printf("Refusing PID <= 1\n"); return 2; }
    task_t task = MACH_PORT_NULL;
    kern_return_t kr = task_for_pid(mach_task_self(), pid, &task);
    printf("TASK PID=%d RESULT=%d\n", pid, kr); fflush(stdout);
    if (kr != KERN_SUCCESS) return 1;
    return sample_task_threads(task);
}
#endif
