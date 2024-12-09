This folder contains a containerized version of Cell2Fire. The container runs the latest version of Cell2Fire without having to manually build and configure the application on your system. The container is compatible with both Podman and Docker. We recommend using Podman, but the instructions found here are easily translated into Docker.

## TL;DR
```
sudo apt install podman git
git clone git@github.com:Cell2Fire/C2F-W.git
cd C2F-W/container
podman build -t c2f .
cd ../test
mkdir results
~/C2F-W/test$ podman run -v $(pwd):/mnt c2f --input-instance-folder /mnt/model/fbp-asc --output-folder /mnt/results --nsims 3 --sim C --grids
ls results

# unnecesary if correctly addressing the output folder:
# enter the container interactively, overriding the entrypoint:
podman run -it --entrypoint /bin/bash c2f
```

## Folder Structure

- `container/`: Contains the files used to build and run the container.
  - `Dockerfile`: Defines the build instructions for the container image.

## Prerequisites

Ensure you have Podman installed on your system. You can find installation instructions in the [official Podman documentation](https://podman.io/docs/installation).

## Building the Image

To build the container image using Podman, navigate to the `container/` directory and run the following command:

```bash
cd container
podman build -t <image_name> -f Dockerfile .
```
This command builds the container image and tags it as <image_name>.

## Running Cell2Fire

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
