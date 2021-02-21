import numpy as np
import pandas as pd
from datetime import date, timedelta, datetime
from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from bs4 import BeautifulSoup


lookup_URL = 'https://www.wunderground.com/history/daily/WSSS/date/{}-{}-{}.html'
start_date = datetime(2020, 1, 1)
end_date = start_date + pd.Timedelta(days=400)

options = webdriver.ChromeOptions()
options.add_argument('headless')
driver = webdriver.Chrome(executable_path='chromedriver', options=options)

data = []

while start_date != end_date:
    print('Gathering data from: ', start_date)
    formatted_lookup_URL = lookup_URL.format(start_date.year,
                                            start_date.month,
                                            start_date.day)
    
    driver.get(formatted_lookup_URL)
    print("Opened URL")

    tables = WebDriverWait(driver,20).until(EC.presence_of_all_elements_located((By.CSS_SELECTOR, "table")))

    soup = BeautifulSoup(driver.page_source, 'lxml')
    tables = soup.find_all('table')
    table = tables[1]

    table_head = table.findAll('th')
    output_head = []
    for head in table_head:
        output_head.append(head.text.strip())

    # ---- Write the rows: ----
    output_rows = []
    rows = table.findAll('tr')
    for j in range(1, len(rows)):
        table_row = rows[j]
        columns = table_row.findAll('td')
        output_row = []
        for column in columns:
            value = column.text.strip()
            output_row.append(value)

        if len(output_row) > 0:
            output_row = [start_date.date()] + output_row
            data.append(output_row)


    start_date += timedelta(days=1)

print("Now, Write the Data.")

output_head = ["Date"] + output_head
df = pd.DataFrame(data, columns = output_head)
df.to_csv("./WeatherData.csv")
print("Raw data has been output as CSV for future use.")

print("Program Complete!")