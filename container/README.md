# Cell2Fire Containers

This folder contains a containerized versions of Cell2Fire. The containers runs the latest version of Cell2Fire without having to manually build and configure the application on your system. 


### Requirement
You only need Podman installed on your system to build and run the container. Replace `podman` with `docker` if needed.
```bash
sudo apt install podman
```

## Usage
After building the container image (named `cell2fire` in the next example), you can run Cell2Fire simulations using the container. The container accepts the same parameters as if you were running the compiled binary directly.

The simplest use is by mounting the instance and results directories into the container using volumes (`-v $(pwd):/mnt`) and deleting the container after the run (`--rm`).
```bash
mkdir results
podman run --rm -v $(pwd):/mnt cell2fire \
    --input-instance-folder /mnt/data/Kitral/Portezuelo-tif \
    --output-folder /mnt/results \
    --output-messages \
    --ignitionsLog \
    --nsims 3 --sim K --grids | tee results/log.txt
ls results
rm -r results/*
```

## Select your preferred build

### 1. Lightweight
```
git clone git@github.com:Cell2Fire/C2F-W.git
cd C2F-W
podman build -t cell2fire -f container/Containerfile .
```

### 2. Run tests too
```
git clone git@github.com:Cell2Fire/C2F-W.git
cd C2F-W
podman build -t cell2firetests -f container/Containerfile+tests .
```

### 3. Minimal dependencies but heavier
Also wget and unzip is needed to get the Dockerfile and a test instance.
```bash
# get the Dockerfile
wget https://github.com/fire2a/C2F-W/raw/main/container/Dockerfile

# build
podman build -t cell2fire -f Dockerfile .

# get an instance
wget https://github.com/fire2a/C2F-W/releases/download/v1.0.1/Kitral-tif.zip
unzip Kitral-tif.zip -d data
```

## Learn More

### Folder Structure

- `container/`: Contains the files used to build and run the container.
  - `Dockerfile`: Defines the build instructions for the container image.

### Prerequisites

Ensure you have Podman installed on your system. You can find installation instructions in the [official Podman documentation](https://podman.io/docs/installation).

### Building the Image

To build the container image using Podman, navigate to the `container/` directory and run the following command:

```bash
cd container
podman build -t <image_name> -f Dockerfile .
```
This command builds the container image and tags it as <image_name>.

### Running Cell2Fire

Once the image is built, you can run the simulation using Podman. It accepts the same parameters as if you were running the compiled binary directly.
In order for the container to have access to the input data files, we must use volumes. Simply put the `-v` or `--volume` tag followed by the path to your input files
(where the fuel and weather files are stored) like this:  `-v <path/to/data>:mnt`. Once the simulation is finished, the results will be saved to that same directory.

```bash
cd <folder with data>
mkdir results 
podman run -v <path/to/data>:/mnt [options] <image_name>  [cell2fire arguments]
```

For example the following command:

```bash
podman run --name <container_name> -v /test/model/kitral-asc:/mnt -dt <image_name> --input-instance-folder /mnt/model/kitral-asc --output-folder /mnt/results --nsims 113  --sim K  
```
would run the Kitral model on a detached container, using data found in `test/model/kitral-asc` for a total of 113 simulations and write the output to `test/model/kitral-asc/results`.

You can access the simulations logs either by adding the `-it` flag to the `podman run` command to run it interactively or by executing:
```bash
podman logs <container_name>
```

A list of options for the `run` command can be found in the [official Podman documentation](https://docs.podman.io/en/latest/markdown/podman-run.1.html).
