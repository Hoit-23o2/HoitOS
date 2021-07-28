from paint import *
from format import *
from global_file import *

def main():
    raw_path = get_local_file()
    sections = get_sections_from_raw(raw_path)
    name_sections(sections)
    table_name = give_table_name()
    
    out_path = format(raw_path, sections, table_name)
    [table_name, col_names, col_values] = parse_formatted_file(out_path)
    draw(table_name, col_names, col_values)
    
if __name__ == '__main__':
    main()