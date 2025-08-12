%.o: %.c
	@$(ECHO) compiling $*.c
	@"$(X86_64-ELF-GCC)" $(CFLAGS) -MP -MD -MF $*.dep -c -o $*.o $*.c

%.o: %.S
	@$(ECHO) compiling $*.S
	@"$(X86_64-ELF-GCC)" $(AFLAGS) -MP -MD -MF $*.dep -c -o $*.o $*.S