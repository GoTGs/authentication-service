FROM gcc

WORKDIR /server/

COPY . .

RUN apt-get update

RUN apt-get install -y cmake

RUN apt-get install -y libpq-dev
RUN apt-get install -y libxslt-dev libxml2-dev libpam-dev libedit-dev libssl-dev

RUN cmake -S . -B build
RUN cmake --build build

RUN mkdir /app
RUN cp -r build/* /app

EXPOSE 8000

CMD [ "stdbuf", "-oL", "./build/auth" ]