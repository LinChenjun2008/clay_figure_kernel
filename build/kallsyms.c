#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    void *address;
    char type;
    char *symbol;
} symbol_entry_t;

static void read_symbol(FILE *fp,symbol_entry_t *sym_entry)
{
    char string[100];
    int rc;

    rc = fscanf(fp,"%p %c %100s\n",&sym_entry->address,&sym_entry->type,string);

    sym_entry->symbol = strdup(string);
}

void read_map(FILE *fp,int *count,symbol_entry_t **ptable)
{
    int size = 0;
    symbol_entry_t *table = *ptable;
    while(!feof(fp))
    {
        if(*count >= size)
        {
            size += 50;
            table = realloc(table,sizeof(*table) * size);
            *ptable = table;
        }
        read_symbol(fp,&table[(*count)++]);
    }
}

void write_src(symbol_entry_t *table,int *count)
{
    printf("void *kallsyms_address[] =\n{");
    int i;
    for(i = 0;i < *count;i++)
    {
        printf("\n\t(void*)%p,",table[i].address);
    }
    printf("\n};\n");
    printf("const char *kallsyms_symbols[] =\n{");
    for(i = 0;i < *count;i++)
    {
        printf("\n\t\"%s\",",table[i].symbol);
    }
    printf("\n};\n");
    printf("\nint kallsyms_count = %d;\n",*count);
    return;
}

int main(int argc,char **argv)
{
    int count = 0;

    symbol_entry_t *table = NULL;

    read_map(stdin,&count,&table);

    write_src(table,&count);
    free(table);
    return 0;
}