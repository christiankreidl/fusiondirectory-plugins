<?php

require_once('/usr/share/phpqrcode/qrlib.php');
require_once('otphp/OTPInterface.php');
require_once('otphp/TOTPInterface.php');
require_once('otphp/ParameterTrait.php');
require_once('otphp/OTP.php');
require_once('otphp/TOTP.php');
require_once('otphp/Factory.php');
require_once('beberlei/Assert/Assertion.php');
require_once('ParagonIE/ConstantTime/Binary.php');
require_once('ParagonIE/ConstantTime/EncoderInterface.php');
require_once('ParagonIE/ConstantTime/Base32.php');
require_once('beberlei/Assert/AssertionFailedException.php');
require_once('beberlei/Assert/InvalidArgumentException.php');
