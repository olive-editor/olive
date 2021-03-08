FROM alpine:latest AS ci-docs

RUN apk update

RUN apk add \
    git \
    python3 \
    py3-pip

RUN python3 -m pip install --upgrade pip

RUN mkdir -p /opt/docs

COPY docs/requirements.txt \
     /opt/docs

RUN cd /opt && \
    python3 -m pip install -r docs/requirements.txt && \
    rm -rf docs

VOLUME /opt/docs
WORKDIR /opt/docs

EXPOSE 8000

CMD [ "sh", "-c", "sphinx-build . _build -b html -j auto && python3 -m http.server --directory _build/html" ]
