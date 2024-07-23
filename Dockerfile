FROM gcc AS development

WORKDIR /server/

RUN apt-get update

RUN apt-get install -y cmake libpq-dev libxslt-dev libxml2-dev libpam-dev libedit-dev libssl-dev

ARG PG_USER
ARG PG_PASS
ARG PG_DB
ARG PG_HOST
ARG PG_PORT
ARG RSASECRET
ARG RSAPUBLIC

ENV PG_USER=${PG_USER}
ENV PG_PASS=${PG_PASS}
ENV PG_DB=${PG_DB}
ENV PG_HOST=${PG_HOST}
ENV PG_PORT=${PG_PORT}
ENV RSASECRET=${RSASECRET}
ENV RSAPUBLIC=${RSAPUBLIC}

COPY dependencies .

COPY . .

FROM development AS builder

RUN cmake -S . -B build
RUN cmake --build build

RUN mkdir /app
RUN cp -r build/* /app

FROM builder AS production

EXPOSE 8000

CMD [ "stdbuf", "-oL", "./build/auth" ]
