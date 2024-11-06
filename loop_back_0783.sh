#!/usr/bin/env bash


SESSION_NAME=benchmark
sudo tmux ls |grep ${SESSION_NAME} && tmux kill-session -t ${SESSION_NAME}
sudo tmux new -s $SESSION_NAME -d

sudo tmux split-window -t $SESSION_NAME -d


for i in $(seq 0); do
    sudo tmux next-layout -t $SESSION_NAME
done

#sudo tmux send-keys -t ${SESSION_NAME}.0 "export LD_LIBRARY_PATH=/home/astrojhgu/usr/lib/x86_64-linux-gnu/" C-M

sudo tmux send-keys -t ${SESSION_NAME}.0 "./recv -l 0 -a 0000:05:00.1 --file-prefix=recv1" 

#sudo tmux send-keys -t ${SESSION_NAME}.1 "export LD_LIBRARY_PATH=/home/astrojhgu/usr/lib/x86_64-linux-gnu/" C-M

sudo tmux send-keys -t ${SESSION_NAME}.1 "./send -l 2 -a 0000:05:00.0 --file-prefix=send1 -- send_cfg0_0783.yaml"

#sudo tmux attach -t ${SESSION_NAME}

