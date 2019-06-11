FROM python:3-alpine

WORKDIR /test

COPY requirements.txt .

RUN pip install --no-cache-dir -r requirements.txt

COPY test_integration.py /test

CMD ["pytest", "-o", "log_cli=true"]
