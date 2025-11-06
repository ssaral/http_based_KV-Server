FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y build-essential cmake libpq-dev libpqxx-dev git wget

WORKDIR /app
COPY . .

# Download httplib header
RUN mkdir -p include && wget https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h -O include/httplib.h

RUN cmake -S . -B build && cmake --build build

CMD ["build/server"]
