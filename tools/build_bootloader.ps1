param(
    [string]$ToolchainBin = ""
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot "Bootloader\build"

if ([string]::IsNullOrWhiteSpace($ToolchainBin)) {
    $gccCommand = Get-Command arm-none-eabi-gcc.exe -ErrorAction SilentlyContinue
    if ($null -eq $gccCommand) {
        throw "arm-none-eabi-gcc.exe not found. Pass -ToolchainBin with the GNU Arm bin directory."
    }
    $ToolchainBin = Split-Path -Parent $gccCommand.Source
}

$gcc = Join-Path $ToolchainBin "arm-none-eabi-gcc.exe"
$objcopy = Join-Path $ToolchainBin "arm-none-eabi-objcopy.exe"
$sizeTool = Join-Path $ToolchainBin "arm-none-eabi-size.exe"
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

$common = @(
    "-mcpu=cortex-m3", "-mthumb", "-Os", "-ffunction-sections",
    "-fdata-sections", "-Wall", "-DSTM32F103xE",
    "-I$repoRoot\Core\Inc",
    "-I$repoRoot\Drivers\CMSIS\Device\ST\STM32F1xx\Include",
    "-I$repoRoot\Drivers\CMSIS\Include"
)

& $gcc @common -c "$repoRoot\Bootloader\bootloader_main.c" -o "$buildDir\bootloader_main.o"
& $gcc @common -c "$repoRoot\Core\Src\system_stm32f1xx.c" -o "$buildDir\system_stm32f1xx.o"
& $gcc @common -x assembler-with-cpp -c "$repoRoot\Core\Startup\startup_stm32f103retx.s" -o "$buildDir\startup.o"
$linkArgs = @(
    "-mcpu=cortex-m3", "-mthumb",
    "-T$repoRoot\Bootloader\STM32F103RETX_BOOTLOADER.ld",
    "-Wl,--gc-sections", "-Wl,-Map=$buildDir\wbee_bootloader.map",
    "--specs=nano.specs", "--specs=nosys.specs",
    "$buildDir\startup.o", "$buildDir\system_stm32f1xx.o",
    "$buildDir\bootloader_main.o", "-o", "$buildDir\wbee_bootloader.elf"
)
& $gcc @linkArgs
& $objcopy -O ihex "$buildDir\wbee_bootloader.elf" "$buildDir\wbee_bootloader.hex"
& $sizeTool "$buildDir\wbee_bootloader.elf"
Write-Host "Bootloader HEX: $buildDir\wbee_bootloader.hex"
