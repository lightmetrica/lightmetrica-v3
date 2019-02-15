#!/bin/bash
docker run \
  -it --rm -p 8888:8888 \
  --user root \
  -h $(hostname) \
  --cap-add=SYS_PTRACE \
  --security-opt seccomp=unconfined \
  -e NB_UID=`id -u $USER` \
  -e GRANT_SUDO=yes \
  -v $(cat .lm3_workspace):/home/jovyan/work \
  -v $(cat .lm3_scene):/home/jovyan/scenes \
  lm3_dev start-notebook.sh \
    --NotebookApp.token='haidomo' \
    --config=/home/jovyan/work/notebook.json \
    --ip=0.0.0.0 \
    --no-browser
