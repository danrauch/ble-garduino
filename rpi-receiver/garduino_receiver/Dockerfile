FROM python:buster

WORKDIR /usr/src/app

RUN apt-get update && apt-get install -y \
    make \
    g++ \
    bluetooth \
    bluez \
    libbluetooth-dev

COPY requirements.txt ./
RUN pip install --no-cache-dir -r requirements.txt

COPY . .

CMD [ "python3", "./garduino_receiver.py" ]