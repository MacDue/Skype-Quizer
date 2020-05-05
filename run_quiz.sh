#!/bin/bash
bash -c "cd ./build/ && ./quiz" & bash -c "cd ./controller && npm start" && fg; fg
