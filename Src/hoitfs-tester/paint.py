from typing import List, Tuple

from xlwings.main import Book, Sheet
from global_var import *
import xlwings

import win32com.client      
from PIL import ImageGrab   
import os
import sys
def save_chart(in_excel_path:str, out_img_path:str):
    # Open the excel application using win32com
    o = win32com.client.Dispatch("Excel.Application")
    # Disable alerts and visibility to the user
    o.Visible = 0
    o.DisplayAlerts = 0
    # Open workbook
    wb = o.Workbooks.Open(in_excel_path)

    # Extract first sheet
    sheet = o.Sheets(1)
    for n, shape in enumerate(sheet.Shapes):
        # Save shape to clipboard, then save what is in the clipboard to the file
        shape.Copy()
        image = ImageGrab.grabclipboard()
        try:
        # Saves the image into the existing png file (overwriting) TODO ***** Have try except?
            image.save(out_img_path, 'png')
        except Exception as e:
            print(e)
        pass
    pass

    wb.Close(True)
    o.Quit()

def is_number(str_number: str):
    return (str_number.split(".")[0]).isdigit() or str_number.isdigit() or  (str_number.split('-')[-1]).split(".")[-1].isdigit()

def parse_formatted_file(raw_path: str) -> Tuple[str, int ,List[List]]:
    with open(raw_path) as f:
        G_VAR_DATA = f.readlines()
        table_name = ""
        col_names = []
        col_values = []
        col_cnt = -1
        
        f_table_name_start = False
        f_col_names_start = False
        f_col_values_start = False
        
        for line in G_VAR_DATA:
            line_striped: str = line.strip()
            
            """ 状态机结束，和执行 """
            if f_table_name_start == True:
                if line_striped == G_SYM_SECTION_DIVIDER:
                    f_table_name_start = False
                    continue
                else:
                    table_name = line_striped

            elif f_col_names_start == True:
                if line_striped == G_SYM_SECTION_TITLE_DEIVIDER:
                    f_col_names_start = False
                    continue
                else:
                    col_names = line_striped.split(" ")
                    if col_cnt == -1:
                        col_cnt = len(col_names)
                        for i in range(col_cnt):
                            col_values.append([])
                    
            
            elif f_col_values_start == True:
                if line_striped == G_SYM_CONTENT_DIVIDER:
                    f_col_values_start = False
                    continue
                else:
                    value:List[str] = line_striped.split(G_SYM_SEP)
                    real_value = []
                    for token in value:
                        if is_number(token):
                            real_value.append(token)
                    for i in range(col_cnt):
                        col_values[i].append(real_value[i])
            """ 状态机开始 """
            if line_striped == G_SYM_SECTION_DIVIDER:
                f_table_name_start = True
                continue

            if line_striped == G_SYM_SECTION_TITLE_DEIVIDER:
                f_col_names_start = True
                continue

            if line_striped == G_SYM_CONTENT_DIVIDER:
                f_col_values_start = True
                continue
        print(table_name)
        print(col_names)
        print(col_values)
        return (table_name, col_names, col_values)

def draw(table_name: str, col_names: List[str], col_values: List[List[str]]):
    root_dir = os.path.dirname(__file__)
    base_out_img_dir = root_dir + G_IMG_OUT_DIR
    base_out_excels_dir = root_dir + G_EXCEL_OUT_DIR
    out_img_path = root_dir + G_IMG_OUT_DIR + "\\img_"+table_name +".png"
    out_xlsx_path = root_dir + G_EXCEL_OUT_DIR + "\\excel_"+table_name +".xlsx"
    
    if not os.access(base_out_img_dir, os.F_OK):
        os.mkdir(base_out_img_dir)
    
    if not os.access(base_out_excels_dir, os.F_OK):
        os.mkdir(base_out_excels_dir)
    
    wb = xlwings.Book()
    sheet : Sheet = wb.sheets[0]  
    sheet_value = []
    col_cnt = len(col_names)
    row_cnt = len(col_values[0])
    sheet_value.append(col_names)
    
    for i in range(row_cnt):
        row = []
        for j in range(col_cnt):
            row.append(col_values[j][i])
        sheet_value.append(row)

    print(sheet_value)

    sheet.range("A1").value = sheet_value

    chart = sheet.charts.add(100, 100)
    chart.set_source_data(sheet.range("A1").expand())
    chart.chart_type = "line"

    chart.api[1].SetElement(2) 
    chart.api[1].ChartTitle.Text = table_name
    

    wb.save(out_xlsx_path)
    wb.close()
    save_chart(out_xlsx_path, out_img_path)


def main():
    [table_name, col_names, col_values] = parse_formatted_file("./data2/test_read.txt")
    draw(table_name, col_names, col_values)
if __name__ == "__main__":
    main()