![license](https://img.shields.io/github/license/IW4x/iw6x-client.svg)
[![Build status](https://ci.appveyor.com/api/projects/status/a7b3tu814330cdkd/branch/master?svg=true)](https://ci.appveyor.com/project/momo5502/iw6x-client/branch/master)
[![discord](https://img.shields.io/endpoint?url=https://momo5502.com/iw4x/members-badge.php)](https://discord.gg/sKeVmR3)
[![patreon](https://img.shields.io/badge/patreon-support-blue.svg?logo=patreon)](https://www.patreon.com/iw4x)

# IW6x: Client

![iw4x](src/client/resources/logo.bmp?raw=true)

## Download

- Grab the latest build from <a href="https://ci.appveyor.com/api/projects/momo5502/iw6x-client/artifacts/build%2Fbin%2Fx64%2FRelease%2Fiw6x.exe?branch=master&job=Environment%3A%20APPVEYOR_BUILD_WORKER_IMAGE%3DVisual%20Studio%202019%2C%20PREMAKE_ACTION%3Dvs2019%3B%20Configuration%3A%20Release">here</a> and drop it in your Call of Duty: Ghosts installation folder.
- The client is still in an early alpha stage. It is far from being usable and will have bugs!  

## How to compile

- Run `premake5 vs2019` or use the delivered `generate.bat`.
- Build via solution file in `build\iw6x.sln`.

## Premake arguments

| Argument                    | Description                                    |
|:----------------------------|:-----------------------------------------------|
| `--copy-to=PATH`            | Optional, copy the EXE to a custom folder after build, define the path here if wanted. |
| `--dev-build`               | Enable development builds of the client. |
## Disclaimer

This software has been created purely for the purposes of
academic research. It is not intended to be used to attack
other systems. Project maintainers are not responsible or
liable for misuse of the software. Use responsibly.
