FROM python:3.11-alpine

ARG SOURCE
COPY ${SOURCE} /function.py
COPY runner.py /runner.py

ENTRYPOINT ["python3", "/runner.py", "/function.py"]
