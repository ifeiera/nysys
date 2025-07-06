$paths = @("src", "include")

$extensions = @("*.cpp", "*.hpp", "*.c", "*.h")

foreach ($path in $paths) {
    foreach ($ext in $extensions) {
        Get-ChildItem -Path $path -Recurse -Include $ext -File | ForEach-Object {
            Write-Host "Formatting $($_.FullName)"
            clang-format -i $_.FullName
        }
    }
}
