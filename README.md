# Parallel-Montecarlo-for-.STL-model-in-C-OpenMP

This project estimates the volume of 3D models stored in STL files using a Monte Carlo method implemented in C with OpenMP. Python helper scripts are used as a numerical reference to compare area/volume and execution times.

## Compile and run C/OpenMP code

```bash
make

./bin/lector-single  data/FILE.stl <NUM-THREADS>
./bin/lector-parallel data/FILE.stl <NUM-THREADS>
```

## Use the Python3 scripts

```bash
python3 -m venv .venv
source .venv/bin/activate        
pip install -r requirements.txt
python3 scripts/TrimeshCalculo.py data/FILE.stl
```
