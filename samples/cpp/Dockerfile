FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y g++ && rm -rf /var/lib/apt/lists/*

ARG SOURCE

COPY ${SOURCE} /src/

RUN g++ -O2 -o /function /src/${SOURCE}

ENTRYPOINT ["/function"]
