#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MEM_SIZE 16384 // MUST equal PAGE_SIZE * PAGE_COUNT
#define PAGE_SIZE 256  // MUST equal 2^PAGE_SHIFT
#define PAGE_COUNT 64
#define PAGE_SHIFT 8 // Shift page number this much

#define PTP_OFFSET 64 // How far offset in page 0 is the page table pointer table

// Simulated RAM
unsigned char mem[MEM_SIZE];

//
// Convert a page,offset into an address
//
int get_address(int page, int offset)
{
    return (page << PAGE_SHIFT) | offset;
}

//
// Initialize RAM
//
void initialize_mem(void)
{
    memset(mem, 0, MEM_SIZE);

    int zpfree_addr = get_address(0, 0);
    mem[zpfree_addr] = 1; // Mark zero page as allocated
}

//
// Get the page table page for a given process
//
unsigned char get_page_table(int proc_num)
{
    int ptp_addr = get_address(0, PTP_OFFSET + proc_num);
    return mem[ptp_addr];
}

//
// Allocate pages for a new process
//
int allocate_page(void)
{
    for (int i = 1; i < PAGE_COUNT; i++)
    {
        int addr = get_address(0, i);
        if (mem[addr] == 0)
        {
            mem[addr] = 1;
            return i;
        }
    }
    return -1; // no free pages
}
// This includes the new process page table and page_count data pages.
//
void new_process(int proc_num, int page_count)
{
    int ptp_page = allocate_page();
    if (ptp_page == -1)
    {
        printf("failed allocation %d: page table\n", proc_num);
        return;
    }

    // add proc_num to the page table
    int ptp_addr = get_address(0, PTP_OFFSET + proc_num);
    mem[ptp_addr] = ptp_page;

    // allocate page_count pages for the process
    for (int i = 0; i < page_count; i++)
    {
        int page = allocate_page();
        if (page == -1)
        {
            printf("failed allocation %d: data page %d\n", proc_num, i);
            return;
        }
        // set the page table entry to the page
        int addr = get_address(ptp_page, i);
        mem[addr] = page;
    }
}

// kill a process
void kill_process(int proc_num)
{
    int ptp_page = get_page_table(proc_num);
    if (ptp_page == 0)
    {
        printf("failed to kill %d: none\n", proc_num);
        return;
    }
    // free the page table
    int ptp_addr = get_address(0, PTP_OFFSET + proc_num);
    mem[ptp_addr] = 0;
    // free the data
    for (int i = 0; i < PAGE_COUNT; i++)
    {
        int addr = get_address(ptp_page, i);
        int page = mem[addr];
        // free the page
        mem[addr] = 0;
        // free the data
        if (page != 0)
        {
            int addr = get_address(0, page);
            mem[addr] = 0;
        }
    }
    // free the page table page
    int addr = get_address(0, ptp_page);
    mem[addr] = 0;
}

//
// Print the free page map
//
// Don't modify this
//
void print_page_free_map(void)
{
    printf("--- PAGE FREE MAP ---\n");

    for (int i = 0; i < 64; i++)
    {
        int addr = get_address(0, i);

        printf("%c", mem[addr] == 0 ? '.' : '#');

        if ((i + 1) % 16 == 0)
            putchar('\n');
    }
}

//
// Print the address map from virtual pages to physical
//
// Don't modify this
//
void print_page_table(int proc_num)
{
    printf("--- PROCESS %d PAGE TABLE ---\n", proc_num);

    // Get the page table for this process
    int page_table = get_page_table(proc_num);

    // Loop through, printing out used pointers
    for (int i = 0; i < PAGE_COUNT; i++)
    {
        int addr = get_address(page_table, i);

        int page = mem[addr];

        if (page != 0)
        {
            printf("%02x -> %02x\n", i, page);
        }
    }
}

// MUST OUTPUT
// printf("Store proc %d: %d => %d, value=%d\n",
//     proc_num, vaddr, addr, val);
void store_value(int proc_num, int vaddr, int val)
{
    int page_table = get_page_table(proc_num);
    // get the vpn
    int virtual_page = vaddr >> PAGE_SHIFT;
    int offset = vaddr & (PAGE_SIZE - 1);

    int phys_page = mem[get_address(page_table, virtual_page)];
    int addr = get_address(phys_page, offset);

    mem[addr] = val;
    printf("Store proc %d: %d => %d, value=%d\n", proc_num, vaddr, addr, val);
}

// MUST OUTPUT
// printf("Load proc %d: %d => %d, value=%d\n",
//     proc_num, vaddr, addr, val);
int load_value(int proc_num, int vaddr)
{
    int page_table = get_page_table(proc_num);

    int virtual_page = vaddr >> PAGE_SHIFT;
    int offset = vaddr & (PAGE_SIZE - 1);

    int phys_page = mem[get_address(page_table, virtual_page)];
    int addr = get_address(phys_page, offset);

    int val = mem[addr];
    printf("Load proc %d: %d => %d, value=%d\n", proc_num, vaddr, addr, val);
    return val;
}

//
// Main -- process command line
//
int main(int argc, char *argv[])
{
    assert(PAGE_COUNT * PAGE_SIZE == MEM_SIZE);

    if (argc == 1)
    {
        fprintf(stderr, "usage: ptsim commands\n");
        return 1;
    }

    initialize_mem();

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "pfm") == 0)
        {
            print_page_free_map();
        }
        else if (strcmp(argv[i], "ppt") == 0)
        {
            int proc_num = atoi(argv[++i]);
            print_page_table(proc_num);
        }
        else if (strcmp(argv[i], "np") == 0)
        {
            int process_num = atoi(argv[++i]);
            int page_count = atoi(argv[++i]);
            new_process(process_num, page_count);
        }
        else if (strcmp(argv[i], "kp") == 0)
        {
            int process_num = atoi(argv[++i]);
            kill_process(process_num);
        }
        else if (strcmp(argv[i], "sb") == 0)
        { // For process n at virtual address a, store the value b.
            int process_num = atoi(argv[++i]);
            int vaddr = atoi(argv[++i]);
            int val = atoi(argv[++i]);
            store_value(process_num, vaddr, val);
        }
        else if (strcmp(argv[i], "lb") == 0)
        { // For process n, get the value at virtual address a.
            int process_num = atoi(argv[++i]);
            int vaddr = atoi(argv[++i]);
            load_value(process_num, vaddr);
        }
        else
        {
            fprintf(stderr, "unknown command: %s\n", argv[i]);
            return 1;
        }
    }
}
