The project uses git submodules:
In order to build the project you need first to checkout the rive-runtime submodule

The command for checking out submodules is
```bash
git submodule update --init --recursive
```
The command should be run from the project root directory.
This will:

- Initialize the `rive-runtime` submodule
- Clone the repository into `submodules/rive-runtime`
- Pull all necessary files including the required `build/build_rive.sh` script
- Initialize any nested submodules within rive-runtime

## Configuration of git checkout
The original rive file is configured to use ssh:
see file [gitmodules](../.gitmodules)
After changing the content of .gitmodules to use https,
you need to run the commands
```bash
git submodule sync
git submodule update --init --recursive
```
