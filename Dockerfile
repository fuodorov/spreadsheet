FROM openjdk:11 as build

RUN apt-get update && \
    apt-get install -y \
    g++ \
    cmake

WORKDIR /spreadsheet

COPY src/ ./src/

WORKDIR /spreadsheet/build

RUN cmake ../src && \
    cmake --build .

FROM ubuntu:latest

RUN apt-get update && \
    apt-get install -y \
    g++ \
    cmake

COPY --from=build \
    ./spreadsheet/build/spreadsheet \
    ./app/

WORKDIR /app