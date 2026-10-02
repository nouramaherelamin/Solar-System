$lines = Get-Content '.\$olar$ystem_impl.cpp'
for($i=0; $i -lt $lines.Length; $i++){
    if($lines[$i] -match 'displayNeptuneScene|initAsteroids|Scene Init|startTransitionTo'){
        Write-Output "$($i+1): $($lines[$i].Trim())"
    }
}
