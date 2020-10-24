![license](https://img.shields.io/github/license/XLabsProject/iw6x-client.svg)
[![open bugs](https://img.shields.io/github/issues/XLabsProject/iw6x-client/bug?label=bugs)](https://github.com/XLabsProject/iw6x-client/issues?q=is%3Aissue+is%3Aopen+label%3Abug)
[![Build status](https://ci.appveyor.com/api/projects/status/9v0mvgqxiltl2eai/branch/master?svg=true)](https://ci.appveyor.com/project/XLabsProject/iw6x-client/branch/master)
[![Build](https://github.com/XLabsProject/iw6x-client/workflows/Build/badge.svg)](https://github.com/XLabsProject/iw6x-client/actions)
[![discord](https://img.shields.io/endpoint?url=https://momo5502.com/iw4x/members-badge.php)](https://discord.gg/sKeVmR3)
<!---
[![patreon](https://img.shields.io/badge/patreon-support-blue.svg?logo=patreon)](https://www.patreon.com/iw4x)
-->

# IW6x: Client

![iw4x](src/client/resources/logo.bmp?raw=true)

## Download

- **[Click here to download the latest build](https://ci.appveyor.com/api/projects/XLabsProject/iw6x-client/artifacts/build%2Fbin%2Fx64%2FRelease%2Fiw6x.exe?branch=master&job=Environment%3A%20APPVEYOR_BUILD_WORKER_IMAGE%3DVisual%20Studio%202019%2C%20PREMAKE_ACTION%3Dvs2019%3B%20Configuration%3A%20Release)**
- **You will need to drop this in your Call of Duty: Ghosts installation folder. If you don't have Call of Duty: Ghosts, get those game files first.**
- The client is still in an early alpha stage. It is far from being usable and will have bugs!

## How to compile

- Clone the Git repo. Do NOT download it as ZIP, that won't work.
- Update the submodules and run `premake5 vs2019` or simply use the delivered `generate.bat`.
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
