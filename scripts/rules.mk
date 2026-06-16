%.o: %.c
	@$(ECHO) "CC      $*.c"
	@"$(CC)" $(CFLAGS) -MP -MD -MF $*.dep -c -o $*.o $*.c

%.o: %.S
	@$(ECHO) "AS      $*.S"
	@"$(CC)" $(CFLAGS) -MP -MD -MF $*.dep -c -o $*.o $*.S