#!/usr/bin/env bash

ls -la

language=""

if [ -f "./Makefile" ]; then
    language="c"
elif [ -f "./app/pom.xml" ]; then
    language="java"
elif [ -f "./package.json" ]; then
    language="javascript"
elif [ -f "./requirements.txt" ]; then
    language="python"
elif [ -f "./app/main.bf" ]; then
    language="befunge"
else
    echo "Invalid repository"
    exit 1
fi

if [ -f "./Dockerfile" ]; then
    docker build -t $JOB_BASE_NAME .
else
    cp $JENKINS_HOME/images/$language/Dockerfile.standalone Dockerfile
    docker build -t $JOB_BASE_NAME .
fi

docker tag $JOB_BASE_NAME localhost:5000/$JOB_BASE_NAME
docker push localhost:5000/$JOB_BASE_NAME

if [ -f "./whanos.yml" ]; then
    echo "This image should be deployed to a cluster"
fi
