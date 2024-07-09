FROM gcc

WORKDIR /server/

COPY . .

ENV PG_USER=postgres
ENV PG_PASS=postgres
ENV PG_DB=postgres
ENV PG_HOST=postgres
ENV PG_PORT=5432

RUN apt-get update

RUN apt-get install -y cmake

RUN apt-get install -y libpq-dev
RUN apt-get install -y libxslt-dev libxml2-dev libpam-dev libedit-dev libssl-dev

RUN cmake -S . -B build
RUN cmake --build build

RUN mkdir /app
RUN cp -r build/* /app

EXPOSE 80

CMD [ "stdbuf", "-oL", "./build/auth" ]