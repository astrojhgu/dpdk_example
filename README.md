# Send and receive jumbo frames to benchmark dpdk based NIC program

# Usage
```bash
sudo ./recv -l 8 -a 0000:01:00.1 --file-prefix=recv
```

```bash
sudo ./send -l 0 -a 0000:01:00.0 --file-prefix=send
```
