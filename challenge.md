# Challenges

- [ ] Use super-pages to reduce the number of PTEs in page tables.
- [ ] Unmap the first page of a user process so that dereferencing a null
	pointer will result in a fault. You will have to start the user text
	segment at, for example, 4096, instead of 0.
- [x] Add a system call that reports dirty pages (modified pages) using `PTE_D`
