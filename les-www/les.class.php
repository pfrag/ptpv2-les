<?php
/**
 * A PHP class for querying the IPChronos load estimation service, which can be
 * used as a Web front-end. Use this class as follows:
 *
 * $host = "example.les.server"; //LES IP address or hostname
 * $port = 12345; //Port where LES listens to
 * $proto = LESClient::_PROTO_TCP_; //or _PROTO_UDP_
 * $lec = LESClient::init($host, $port, $proto);
 * echo $lec->getLoad(LESClient::_OUTPUT_JSON_);//or _OUTPUT_RAW_/_OUTPUT_PLAIN_
 *
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


/**
 * Class which represents a LES response.
 */
class LoadInfo {
	var $status;
	var $delayavg;
	var $delaymin;
	var $delaymax;
	var $samples;
	var $load;
}

/**
 * LES client.
 */
class LESClient {
	private $host;
	private $port;
	private $proto;
	private $socket;

	/**
	 * Protocol to communicate with LES.
	 */
	const _PROTO_TCP_ = 1;
	const _PROTO_UDP_ = 2;

	/**
	 * Available output formats.
	 */
	const _OUTPUT_RAW_ = 1;
	const _OUTPUT_JSON_ = 2;
	const _OUTPUT_SHORT_ = 3;

	/**
	 * 5sec socket timeout.
	 */
	const _MSG_TIMEOUT_ = 5;
	const _CNX_TIMEOUT_ = 5;

	/**
	 * Private constructor. Only called inside init().
	 */
	private function __construct($h, $p, $proto) {
		$this->host = $h;
		$this->port = $p;
		$this->proto = $proto;
	}

	/**
	 * Static initialization function. Returns a LESClient object.
	 */
	public static function init($h, $p, $proto) {
		$retval = new LESClient($h, $p, $proto);
		if (!$retval->host = gethostbyname($h)) {
			return NULL;
		}
		
		if ($retval->proto == LESClient::_PROTO_UDP_) {
			$retval->socket = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP);
			if (!$retval->socket) {
				return NULL;
			}
		}
		else {
			$retval->socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
			if (!$retval->socket) {
				return NULL;
			}

			/* set non-blocking mode temporarily */
			socket_set_nonblock($retval->socket);

			/* try a connect and check retval */
			$ret = socket_connect($retval->socket, $retval->host, $retval->port);
			if (!$ret && socket_last_error() != 115) {
				/* some kind of error other than EINPROGRESS */
				return NULL;
			}

			/* now wait for some time... */
			$ret = LesClient::_timeoutCnx($retval->socket, LESClient::_CNX_TIMEOUT_);
			if ($ret === FALSE || $ret <= 0) {
				/* timeout or error */
				return NULL;
			}

			/* socket writable, proceed with checking if connect completed ok */
			if (socket_get_option($retval->socket, SOL_SOCKET, SO_ERROR)) {
				return NULL;
			}

			/* reset blocking mode */
			socket_set_block($retval->socket);
		}
		return $retval;
	}

	/**
	 * Communicate with LES, receive load estimate, and return a LoadInfo
	 * object for further processing (parsing, output).
	 */
	private function _getLoad() {
		$msg = "LREQ\r\nContent-length: 0\r\n";

		/* send mesg */
		socket_sendto($this->socket, $msg, strlen($msg), 0, $this->host, $this->port);

		/* check for timeout */
		$ret = LESClient::_timeoutRcv($this->socket, LESClient::_MSG_TIMEOUT_);
		if ($ret === false || $ret <= 0) {
			return NULL;
		}

		/* receive response */
		socket_recvfrom($this->socket, $resp, 1024, 0, $this->host, $this->port);

		/* parse load response */
		$r = new LoadInfo();
		$clen = 0;

		if (sscanf(strtolower($resp), "lrsp\r\ncontent-length: %d\r\nstatus: %d\r\ndelay-avg: %lf\r\ndelay-min: %Ld\r\ndelay-max: %Ld\r\nsamples: %d\r\nload-type: %lf\r\n", $clen, $r->status, $r->delayavg, $r->delaymin, $r->delaymax, $r->samples, $r->load) != 7) {
			return NULL;
		}
		return $r;
	}

	/**
	 * JSON output.
	 */
	private function _toJSON($linfo) {
		if ($linfo) {
			$arr = array("Status" => $linfo->status, "Delay-avg" => $linfo->delayavg, "Delay-min" => $linfo->delaymin, "Delay-max" => $linfo->delaymax, "Samples" => $linfo->samples, "Load-type" => $linfo->load);
			return json_encode($arr);
		}
		else {
			return json_encode(array("Status" => "400"));
		}
	}

	/**
	 * Output only the estimated load (a single float number)
	 */
	private function _toShortOutput($linfo) {
		if ($linfo) {
			return $linfo->load;
		}
		else {
			return -1;
		}
	}

	/**
	 * Raw output (original received LRSP message).
	 */
	private function _toRawOutput($linfo) {
		if ($linfo) {
			$content = "Status: $linfo->status\r\nDelay-avg: $linfo->delayavg\r\nDelay-min: $linfo->delaymin\r\nDelay-max: $linfo->delaymax\r\nSamples: $linfo->samples\r\nLoad-type: $linfo->load\r\n";
		}
		else {
			$content = "Status: 400\r\n";
		}
		$clen = strlen($content);
		return "LRSP\r\nContent-length: " . $clen . "\r\n" . $content;
	}

	/**
	 * Query LES, and output appropriately formatted response.
	 */
	public function getLoad($format) {
		$linfo = $this->_getLoad();
		switch ($format) {
			case LESClient::_OUTPUT_SHORT_:
				return $this->_toShortOutput($linfo);
			case LESClient::_OUTPUT_RAW_:
				return $this->_toRawOutput($linfo);
			case LESClient::_OUTPUT_JSON_: default:
				return $this->_toJSON($linfo);
		}
	}

	/**
	 * Destructor. Closes socket.
	 */
	public function __destruct() {
		@socket_close($this->socket);
	}

	/**
	 * Timeout functions
	 */
	private static function _timeoutRcv($sock, $sec) {
		$readset = array($sock);
		$writeset = array();
		$eset = array();
		return socket_select($readset, $writeset, $eset, $sec);
	}

	private static function _timeoutCnx($sock, $sec) {
		$readset = array();
		$writeset = array($sock);
		$eset = array();
		return socket_select($readset, $writeset, $eset, $sec);
	}

}

?>
