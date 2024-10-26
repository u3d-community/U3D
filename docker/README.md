
## Build Docker Images

This folder contains Dockerfiles for building Docker images to use locally or with GitHub workflows.

Once you have built the Docker images on your system, you can simply run `./script/dockerized.sh`, and it will use these local images.

Currently, the following Dockerfiles are available:

### Android Build:

    To build the Docker image, you will need 6 GB of free space and Docker installed.
    Then, open a terminal in the `u3d-android` subfolder and run:
    ```bash
    docker build -t u3d-android .
    ```
    (You’ll need to prefix this command with sudo if your Docker user permissions are not configured.)
