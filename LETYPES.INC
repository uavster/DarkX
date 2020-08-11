;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
; file: letypes.inc
;
;  Data types for LE loader. Used by TX32 extender.
;
;       by Xavier Rubio Jansana, a.k.a. Teknik / #Trouble# Team, July 1998
;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴

LEHEADER        struc

signature       dw      ?       ; LE indicates Linear Executable
byte_order      db      ?       ; Byte order
word_order      db      ?       ; Word order
exe_level       dd      ?       ; Executable format level
cpu_type        dw      ?       ; CPU type (2 - Intel 386, 3 ...)
target_os       dw      ?       ; Target OS (1 - OS/2)
module_version  dd      ?       ; Module version
module_type     dd      ?       ; Module type
memory_pages    dd      ?       ; Number of memory pages
object_cs       dd      ?       ; CS (object number)
offset_eip      dd      ?       ; EIP (offset)
object_ss       dd      ?       ; SS (object number)
offset_esp      dd      ?       ; ESP (offset)
page_size       dd      ?       ; Memory page size (4 kb)
bytes_last_page dd      ?       ; Bytes on last page
fixup_sect_size dd      ?       ; Fixup section size
fixup_checksum  dd      ?       ; Fixup section checksum
load_sect_size  dd      ?       ; Loader section size
load_checksum   dd      ?       ; Loader section checksum
off_object_tbl  dd      ?       ; Offset of object table
obj_tbl_entries dd      ?       ; Object table entries
off_page_map    dd      ?       ; Object page map table offset
off_obj_it_data dd      ?       ; Object iterate data map offset
off_res_table   dd      ?       ; Resource table offset
res_tbl_entries dd      ?       ; Resource table entries
off_res_names   dd      ?       ; Resident names table offset
off_entry_table dd      ?       ; Entry table offset
off_mod_dirs    dd      ?       ; Module directives table offset
num_mod_dirs    dd      ?       ; Module directives entries
off_fixup_pag   dd      ?       ; Fixup page table offset
off_fixup_rec   dd      ?       ; Fixup record table offset
off_import_mods dd      ?       ; Imported modules name table offset
num_import_mods dd      ?       ; Imported modules count
off_import_proc dd      ?       ; Imported procedures name table offset
off_pages_chksm dd      ?       ; Per-page checksum table offset
off_data_pages  dd      ?       ; Data pages offset
num_preload     dd      ?       ; Preload page count
off_nres_names  dd      ?       ; Non-resident names table offset
len_nres_names  dd      ?       ; Non-resident names table length
nres_names_chk  dd      ?       ; Non-resident names checksum
auto_data_obj   dd      ?       ; Automatic data object
off_dbg_nfo     dd      ?       ; Debug information offset
len_dbg_nfo     dd      ?       ; Debug information length
preload_inst_pg dd      ?       ; Preload instance pages number
demand_inst_pg  dd      ?       ; Demand instance pages number
extra_heap      dd      ?       ; Extra heap allocation
leh_reserved    dd      6 dup (?)

                ends

OBJECT          struc

virtual_size    dd      ?       ; Virtual size in bytes
relocation_base dd      ?       ; Relocation base address
object_flags    dd      ?       ; Object flags
page_map_index  dd      ?       ; Page map index
num_pages       dd      ?       ; Number of page map entries
ot_reserved     dd      ?       ; Reserved

                ends

OBJNFO          struc

obj_handles     dd      ?       ; Object handle to free memory
mem_type        db      ?       ; Handle's memory type
obj_offset      dd      ?       ; Object's base address

                ends

