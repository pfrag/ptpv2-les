<?php
/**
 * Web front-end for accessing the load estimation service.
 *
 * Copyright (C) 2013 Pantelis A. Frangoudis <pfrag@aueb.gr>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

require_once('les.class.php');

$format = "";
$proto = "";
$port = "";

if (isset($_REQUEST["format"])) {
	$format = strtolower($_REQUEST["format"]);
}
if (isset($_REQUEST["proto"])) {
	$proto = strtolower($_REQUEST["proto"]);
}
if (isset($_REQUEST["port"])) {
	$port = strtolower($_REQUEST["port"]);
}

switch($format) {
	case "short":
		$format = LESClient::_OUTPUT_SHORT_;
		break;
	case "raw":
		$format = LESClient::_OUTPUT_RAW_;
		break;
	case "json": default:
		$format = LESClient::_OUTPUT_JSON_;
		break;
}

switch($proto) {
	case "tcp":
		$proto = LESClient::_PROTO_TCP_;
		break;
	case "udp": default:
		$proto = LESClient::_PROTO_UDP_;
		break;
}

/* Check if supplied port number is an integer */
if (filter_var($port, FILTER_VALIDATE_INT) === FALSE) {
	/* invalid port, use default */
	$port = "7575";
}

/* default host: localhost */
$host = "localhost";
if (isset($_REQUEST["host"])) {
	$host = $_REQUEST["host"];
}

$lec = LESClient::init($host, $port, $proto);
if ($lec) {
	echo $lec->getLoad($format);
}
else {
	echo -1;
}
?>
