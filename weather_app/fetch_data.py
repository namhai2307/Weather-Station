import time
from bs4 import BeautifulSoup, element
import requests

def extract_data(url):
    respone = requests.get(url)
    soup = BeautifulSoup(respone.text, "html.parser")
    data = soup.find_all('p')
    print(data[0].text)

while True:
    extract_data("http://192.168.1.167/sensor")
    time.sleep(10)