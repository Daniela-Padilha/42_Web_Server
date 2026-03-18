<?php

echo "<h1>PHP CGI Test</h1>";

echo "<h2>GET parameters</h2>";
print_r($_GET);

echo "<h2>POST parameters</h2>";
print_r($_POST);

echo "<h2>Server variables</h2>";
print_r($_SERVER);

?>