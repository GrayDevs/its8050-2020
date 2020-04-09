# esp-hello-world

## ESP32 development SW installation

This folder contain:
- The report for this task: Report_ESP32_development_Group_G.pdf
- The sample file used during the tutoriel (Step 1-10): hello_world_main.c 
- The 2 optional tasks for extra credits: hello_world/ and hello_world2/

These should only need to be built by running:
```
idf.py built
```

They could then be flashed onto an ESP32 board:
```
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
```