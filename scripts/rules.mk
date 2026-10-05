ifeq ($(OBJ_DIR),)
$(error OBJ_DIR must be defined before including rules.mk)
endif

to_obj = $(addprefix $(OBJ_DIR)/,$(patsubst %.$(2),%.o,$(1)))

$(OBJ_DIR)/%.o: %.c
	@$(MKDIR) -p $(@D)
	@$(ECHO) "CC      $*.c"
	@$(CC) $(CFLAGS) -MP -MD -MF $(patsubst %.dep,%.o,$@) -c -o $@ $<

$(OBJ_DIR)/%.o: %.S
	@$(MKDIR) -p $(@D)
	@$(ECHO) "AS      $*.S"
	@$(CC) $(CFLAGS) -MP -MD -MF $(patsubst %.dep,%.o,$@) -c -o $@ $<