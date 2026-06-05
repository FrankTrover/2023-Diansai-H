Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

if ([System.Windows.Forms.Clipboard]::ContainsImage()) {
    $img = [System.Windows.Forms.Clipboard]::GetImage()
    $path = "C:\Users\THTGZ\Desktop\coding\f407\xiaosai\figures\clipboard_image.png"
    $img.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    Write-Output $path
} else {
    Write-Output "NO_IMAGE"
}
