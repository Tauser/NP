<#
Envia uma credencial Wi-Fi pela USB fisica, sem ecoar a senha no firmware.
Execute com o monitor serial fechado, pois uma porta COM so aceita um cliente.
O frame e base64 para delimitar campos; a confidencialidade vem do cabo USB,
nao de criptografia. Nao grave este script com argumentos contendo senha.
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$Port,

    [ValidateRange(9600, 921600)]
    [int]$BaudRate = 115200,

    [ValidateRange(1, 60)]
    [int]$ObservationSeconds = 15
)

$ssid = Read-Host 'SSID'
$securePassphrase = Read-Host 'Senha Wi-Fi' -AsSecureString
$bstr = [IntPtr]::Zero
$serial = $null
$observedOutput = ''
$accepted = $false
$rejected = $false

try {
    $bstr = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($securePassphrase)
    $passphrase = [Runtime.InteropServices.Marshal]::PtrToStringBSTR($bstr)
    $utf8 = [Text.Encoding]::UTF8
    $ssidEncoded = [Convert]::ToBase64String($utf8.GetBytes($ssid))
    $passphraseEncoded = [Convert]::ToBase64String($utf8.GetBytes($passphrase))
    $frame = "NPW1 $ssidEncoded $passphraseEncoded`n"

    $serial = [IO.Ports.SerialPort]::new($Port, $BaudRate, 'None', 8, 'One')
    $serial.Handshake = [IO.Ports.Handshake]::None
    $serial.Open()
    $serial.Write($frame)
    Write-Host "Credencial enviada. Aguardando a resposta do dispositivo por $ObservationSeconds s..."

    $deadline = [DateTime]::UtcNow.AddSeconds($ObservationSeconds)
    while ([DateTime]::UtcNow -lt $deadline) {
        Start-Sleep -Milliseconds 200
        $deviceOutput = $serial.ReadExisting()
        if ([string]::IsNullOrWhiteSpace($deviceOutput)) {
            continue
        }

        # O firmware nunca ecoa a credencial; este e apenas o log ja publico do dispositivo.
        Write-Host $deviceOutput -NoNewline
        $observedOutput += $deviceOutput
        if ($observedOutput.Length -gt 4096) {
            $observedOutput = $observedOutput.Substring($observedOutput.Length - 4096)
        }

        if ($observedOutput -match 'credencial USB recebida|Wi-Fi associado e com IP') {
            $accepted = $true
        }
        if ($observedOutput -match 'frame USB recusado') {
            $rejected = $true
        }
    }

    if ($rejected) {
        Write-Warning 'O dispositivo recusou o frame. Tente novamente e envie a saida exibida se persistir.'
    }
    elseif ($accepted) {
        Write-Host 'Resposta do dispositivo recebida. Aguarde 30 s com IP estavel para o commit NVS.'
    }
    else {
        Write-Warning 'Nenhuma confirmacao foi lida. Confira a porta COM, deixe o monitor fechado e tente novamente.'
    }
}
finally {
    if ($null -ne $serial) {
        if ($serial.IsOpen) { $serial.Close() }
        $serial.Dispose()
    }
    if ($bstr -ne [IntPtr]::Zero) {
        [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($bstr)
    }
    $passphrase = $null
    $frame = $null
    $observedOutput = $null
}
