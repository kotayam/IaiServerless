FROM alpine:3.20 AS build

RUN apk add --no-cache gcc musl-dev

ARG SOURCE

COPY ${SOURCE} timing.c /src/

RUN gcc -O2 -o /function /src/${SOURCE} /src/timing.c

FROM alpine:3.20

COPY --from=build /function /function

ENTRYPOINT ["/function"]
