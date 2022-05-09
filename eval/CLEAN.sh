#!/bin/bash

DIR=`dirname $0`

find $DIR -name __pycache__* -exec rm -r {} +

find $DIR -name _inputs.zip -delete
find $DIR -name _outputs.zip -delete
find $DIR -name _worker_logs.zip -delete

find $DIR -name _results.db $@ | sort
