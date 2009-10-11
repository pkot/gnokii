#
#  G N O K I I
#
#  A Linux/Unix tool$env:and driver for the mobile phones.
#
#  This file is part of gnokii.
#
#  Gnokii is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  Gnokii is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with gnokii; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#  Copyright (C) 2006-2009       Jari Turkia
#
#  Generate gnokii.h header file from gnokii.h.in
#  Power shell may require: Set-ExecutionPolicy Unrestricted
#

$GNOKII_H_IN_PATH = "..\include"

# .bat: findstr /r /c:"^GNOKII_LT_VERSION_" ..\configure.in
if (!(Test-Path -path "..\configure.in")) {
	Write-Error "Please run this in Gnokii\Windows directory!"
	exit 1
}
$versions = Get-Content "..\configure.in" | Select-String "^GNOKII_LT_VERSION_"

[string]$line = ""
foreach ($line in $versions) {
	$lineData = $line.tostring().split("=")
	switch($lineData[0]) {
		"GNOKII_LT_VERSION_CURRENT" {
			$GNOKII_LT_VERSION_CURRENT = [int]$lineData[1]
		}
		"GNOKII_LT_VERSION_REVISION" {
			$GNOKII_LT_VERSION_REVISION = [int]$lineData[1]
		}
		"GNOKII_LT_VERSION_AGE" {
			$GNOKII_LT_VERSION_AGE = [int]$lineData[1]
		}
	}
}

$LIBGNOKII_VERSION_MAJOR = $GNOKII_LT_VERSION_CURRENT - $GNOKII_LT_VERSION_AGE
$LIBGNOKII_VERSION_MINOR = $GNOKII_LT_VERSION_AGE
$LIBGNOKII_VERSION_RELEASE = $GNOKII_LT_VERSION_REVISION
$LIBGNOKII_VERSION_STRING = "$LIBGNOKII_VERSION_MAJOR.$LIBGNOKII_VERSION_MINOR.$LIBGNOKII_VERSION_RELEASE"

Write-Host "Ver:" $LIBGNOKII_VERSION_STRING

$headerFile = get-content ($GNOKII_H_IN_PATH + "\gnokii.h.in")
$headerFile | Foreach-Object {
	# Don't output anything from first 3.
	$_ = $_ -replace "@LIBGNOKII_VERSION_STRING@", $LIBGNOKII_VERSION_STRING
	$_ = $_ -replace "@LIBGNOKII_VERSION_MAJOR@", $LIBGNOKII_VERSION_MAJOR
	$_ = $_ -replace "@LIBGNOKII_VERSION_MINOR@", $LIBGNOKII_VERSION_MINOR
	# Output the line
	$_ -replace "@LIBGNOKII_VERSION_RELEASE@", $LIBGNOKII_VERSION_RELEASE
} | Out-File -filePath ($GNOKII_H_IN_PATH + "\gnokii.h") -encoding ASCII
