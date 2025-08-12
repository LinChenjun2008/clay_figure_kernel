%.o: %.c
	@$(ECHO) compiling $*.c
	@"$(CC)" $(CFLAGS) -MP -MD -MF $*.dep -c -o $*.o $*.c

%.o: %.S
	@$(ECHO) compiling $*.S
	@"$(CC)" $(AFLAGS) -MP -MD -MF $*.dep -c -o $*.o $*.S