/////////////////////////////////////////////////////////////
// Note: the following code is documented with JSDoc tags. //
/////////////////////////////////////////////////////////////

/** gda is the namespace for the GeoDa/Javascript API.
* For example, to access the curr_sel object, type gda.curr_sel
* @namespace
*/
var gda = gda || {};

/** Current selection hashtable.  Ex, if observation 3 and 30
 * are currently selected, gda.curr_sel should be:
 * { 3: true, 30: true }.
 * This allows us to quickly detect membership in curr_sel.
 * Ex, to test if observation 4 is selected, can write:
 * gda.curr_sel[4] !== undefined;   This call is very fast
 * to execute.  Much faster than (4 in gda.curr_sel) or
 * gda.curr_sel.hasOwnProperty(4);  Emperical testing has
 * shown these last two lookups to be 50x slower.
 * @member {Object}
 * @type {Object.<number, boolean>}
 */
gda.curr_sel = {};

///////////////////////////////////
// BEGIN:   GeoDa JS API Section //
///////////////////////////////////

// Will store mapping from callback ids to callback functions
gda.callback_map = {};

/**
 * Get a unique callback id.
 * @memberof gda
 * @function getCallbackId
 * @returns {string} Unique callback identifier string.
 */
(function () {
  var cb_id = 100; // capture as private variable.
  gda.getCallbackId = function () { return "resp_cb_id_" + cb_id++; };
} ());

/**
* High level definition of a gda Request JSON object
* @typedef {Object} RequestObject
* @property {string} interface - One of "project", "table"
* @property {string} operation - Ex promptVarSettings
*/

/**
* @typedef {Object} VariableSettingsRequestObject
* @property {string} interface - Must be set to project
* @property {string} operation - Must be "promptVarSettings"
* @property {string} arity - One of uni/bi/tri/quadvariate
* @property {boolean} [show_weights=false] - Get a weights matrix
* @property {string} [var1_title="First Variable (X)"] - first variable title
* @property {string} [var2_title="Second Variable (Y)"] - second variable title
* @property {string} [var3_title="Third Variable (Z)"] - third variable title
* @property {string} [var4_title="Fourth Variable"] - fourth variable title
*/

/**
* This callback type is called 'requestCallback'
* @callback requestCallback
* @param {Object} response_obj
*/

/**
* reqs: an array of request objects
* @param {RequestObject[]} reqs
* @param {requestCallback} callback
*/
gda.makeRequests = function (reqs, callback) {
  var a = {};
  a.action = "request";
  a.callback_id = gda.getCallbackId();
  a.requests = reqs;
  gda.callback_map[a.callback_id] = callback;
  document.title = JSON.stringify(a);
};

/** responses_obj */
gda.response = function (responses_obj) {
  // find callback in callback map.
  // if callback not found, then assume request was canceled.
  // Otherwise, execute function associated with callback_id

  var cb_id = responses_obj.callback_id;
  if (!cb_id) return;
  if (!gda.callback_map[cb_id]) return;
  gda.callback_map[cb_id](responses_obj.responses);
  delete gda.callback_map[cb_id];
};

/**
      o is a JSON object with the following format:
      {"observable": "HighlightState",
	   "event": "delta",
	   "newly_highlighted": [3,2,32,4],
	   "newly_unhighlighted": [23, 6, 7]
	   }
      */
gda.update = function (o) {
  gda.logMsg("In gda.update");

  //gda.logMsg(JSON.stringify(o), "update_para");

  if (o.observable === "HighlightState" && gda.updateHS) {
    gda.updateHS(o);
  }
  if (o.observable === "TimeState" && gda.updateTmS) {
    gda.updateTmS(o);
  }
  if (o.observable === "WeightsManState" && gda.updateWS) {
    gda.updateWS(o);
  }
};

/** This is an example function that needs to be
 defined by the programmer */
gda.readyToInit = function () {
  // To immediately closes the web view:
  //   document.title = JSON.stringify({action: "close"});

  /*
  var requests = [];
  // Variable Settings Request Object
  var vs_o = {
    interface: "project",
    operation: "promptVarSettings",
    arity: "bivariate",
    show_weights: false,
    title: "Custom D3/JS Scatterplot Variables",
    var1_title: "x-axis",
    var2_title: "y-axis"
  };
  requests.push(vs_o);

  gda.makeRequests(requests,
                   function (resp_array) {
    // resp_array is the response array.
    // We only expect one response object
    // in this case.
    var o = resp_array[0];
    var initObj = {
      var1_title: o.var1.name,
      var1_data: o.var1.data[0],
      var1_title: o.var2.name,
      var2_data: o.var2.data[0],
      selected: o.selected
    };
    gda.initFromDataset(initObj);
    gda.logMsg("received VS response", "vs_para") });
  */
}

///////////////////////////////////
// END:   GeoDa JS API Section   //
///////////////////////////////////


// Example utility function to get the current window size on resize.
// To have this function called when the browser window is resize,
// can write <body onresize="gda.updateWindow()"> or
// document.body.onresize = gda.updateWindow;
gda.updateWindow = function () {
  var w = window;
  var d = document;
  var e = d.documentElement;
  var g = d.getElementsByTagName('body')[0];
  var x = w.innerWidth || e.clientWidth || g.clientWidth;
  var y = w.innerHeight || e.clientHeight || g.clientHeight;
  gda.logMsg("window size: (" + x + ", " + y + ")", "win_sz_para");
  //var svg = d3.select("body").select("svg");
  //svg.attr("width", x).attr("height", y);
}

gda.getWindowSize = function () {
  var w = window;
  var d = document;
  var e = d.documentElement;
  var g = d.getElementsByTagName('body')[0];
  var x = w.innerWidth || e.clientWidth || g.clientWidth;
  var y = w.innerHeight || e.clientHeight || g.clientHeight;
  //gda.logMsg("window size: (" + x + ", " + y + ")", "win_sz_para");
  return {width: x, height: y};
}

// Utility function to add a log
// message to <body>.  If no id
// given, then default "log_para"
// is used.  Otherwise will replace
// paragraph with id if it exists.
gda.logMsg = function (msg, id) {
  if (!id) id = "log_para";
  var p = document.getElementById(id);
  if (!p) {
    p = document.createElement("p");
    p.id = id;
    document.body.appendChild(p);
  }
  p.innerHTML = msg;
}

// Utility function to add a log
// message to <body>.  If no id
// given, then default "log_para"
// is used.  Similar to gda.logMsg,
// but appends to to existing
// paragraph rather than overwriting.
gda.appendMsg = function (msg, id) {
  if (!id) id = "log_para";
  var p = document.getElementById(id);
  if (!p) {
    p = document.createElement("p");
    p.id = id;
    document.body.appendChild(p);
  }
  p.innerHTML += "<br />" + msg;
}


/** This is an example that needs to be redefined to
do actual highlight/unhighlights. */
gda.updateHS = function (o) {
  gda.logMsg("In gda.updateHS");

  if (o.event === "unhighlight_all") {

  } else if (o.event === "invert") {

  } else if (o.event === "delta") {

  }
}

/** Example function that needs to be redefined.
@abstract
*/
gda.updateTmS = function (o) {
  gda.logMsg("In gda.updateTmS");
  gda.logMsg("time id: " + o.curr_time + ", time name: " + o.curr_time_str, "time_para");
};

/** Example function that needs to be redefined */
gda.updateWS = function (o) {
  gda.logMsg("In gda.updateWS");
  gda.logMsg("weights event: " + o.event);
};

// Merges object A into object B
gda.merge_obj_into_obj = function (A, B) {
  for (var key in A) {
    B[key] = A[key];
  }
};

// Merges array A into object B
gda.merge_array_into_obj = function (A, B) {
  for (var i=0, sz=A.length; i<sz; ++i) {
    B[A[i]] = true;
  }
};

// Remove object A from object B
gda.remove_obj_from_obj = function (A, B) {
  for (var key in A) {
    delete B[key];
  }
};

// Remove array A from object B
gda.remove_array_from_obj = function (A, B) {
  for (var i=0, sz=A.length; i<sz; ++i) {
    delete B[A[i]];
  }
};

// Remove all elements from object A
gda.remove_all_from_obj = function (A) {
  for (var key in A) {
    delete A[key];
  }
};

// Remove all elements from array A
gda.remove_all_from_array = function (A) {
  while(A.length) { A.pop(); }
};

// Calculates A union B
gda.array_union = function (A, B) {
  var map = {};
  var C = [];
  for (var i=0, sz=A.length; i<sz; ++i) C.push(A[i]);
  for (var i=0, sz=B.length; i<sz; ++i) {
    map[A[i]] = null;
  }
  for (var i=0, sz=B.length; i<sz; ++i) {
    if (!map.hasOwnProperty(B[i])) {
      C.push(B[i]);
    }
  }
  return C;
};

// Calculates A - B
gda.array_minus = function (A, B) {
  var map = {};
  var C = [];
  for (var i=0, sz=B.length; i<sz; ++i) {
    map[B[i]] = null;
  }
  for (var i=0, sz=A.length; i<sz; ++i) {
    if (!map.hasOwnProperty(A[i])) {
      C.push(A[i]);
    }
  }
  return C;
};

gda.arrays_equal = function (A, B) {
  var map = {};
  for (var i=0, sz=B.length; i<sz; ++i) {
    map[B[i]] = null;
  }
  for (var i=0, sz=A.length; i<sz; ++i) {
    if (!map.hasOwnProperty(A[i])) return false;
  }
  var map = {};
  for (var i=0, sz=A.length; i<sz; ++i) {
    map[A[i]] = null;
  }
  for (var i=0, sz=B.length; i<sz; ++i) {
    if (!map.hasOwnProperty(B[i])) return false;
  }
  return true;
};
