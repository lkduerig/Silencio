'use strict';

// Create an instance
var wavesurfer = Object.create(WaveSurfer);

// Init & load audio file
document.addEventListener('DOMContentLoaded', function () {
  var options = {
    container     : document.querySelector('#waveform'),
    waveColor     : '#CCC',
    progressColor : '#CCC',
    loaderColor   : '#ff595b',
    cursorColor   : '#ff595b',
    markerWidth   : 2
  };

  if (location.search.match('scroll')) {
    options.minPxPerSec = 100;
    options.scrollParent = true;
  }

  if (location.search.match('normalize')) {
    options.normalize = true;
  }

  /* Progress bar */
  (function () {
    var progressDiv = document.querySelector('#progress-bar');
    var progressBar = progressDiv.querySelector('.progress-bar');

    var showProgress = function (percent) {
      progressDiv.style.display = 'block';
      progressBar.style.width = percent + '%';
    };

    var hideProgress = function () {
      progressDiv.style.display = 'none';
    };

    wavesurfer.on('loading', showProgress);
    wavesurfer.on('ready', hideProgress);
    wavesurfer.on('destroy', hideProgress);
    wavesurfer.on('error', hideProgress);
  }());

  // Init
  wavesurfer.init(options);
  // Load audio from URL
  wavesurfer.load(options.container.attributes.data.value);
});

// Play at once when ready
// Won't work on iOS until you touch the page
wavesurfer.on('ready', function () {
  wavesurfer.play();
  TimePlugin();
});

// Do something when the clip is over
wavesurfer.on('finish', function () {
  console.log('Finished playing');
});

// Bind buttons and keypresses
(function () {
  var eventHandlers = {
    'play': function () {
      wavesurfer.playPause();
    },

    'back': function () {
      wavesurfer.skipBackward();
    },

    'forth': function () {
      wavesurfer.skipForward();
    },

    'toggle-mute': function () {
      wavesurfer.toggleMute();
    }
  };

  document.addEventListener('keydown', function (e) {
    var map = {
      32: 'play',       // space
      37: 'back',       // left
      39: 'forth'       // right
    };
    if (e.keyCode in map && e.currentTarget.activeElement.type != 'text' && e.currentTarget.activeElement.type != 'textarea') {
      var handler = eventHandlers[map[e.keyCode]];
      e.preventDefault();
      handler && handler(e);
    }
  });

  document.addEventListener('click', function (e) {
    var action = e.target.dataset && e.target.dataset.action;
    if (action && action in eventHandlers) {
      eventHandlers[action](e);
    }
  });
}());

// Flash mark when it's played over
wavesurfer.on('mark', function (marker) {
  if (marker.timer) { return; }

  marker.timer = setTimeout(function () {
    var origColor = marker.color;
    marker.update({ color: 'yellow' });

    setTimeout(function () {
      marker.update({ color: origColor });
      delete marker.timer;
    }, 100);
  }, 100);
});

wavesurfer.on('error', function (err) {
  console.error(err);
});

// Drag'n'drop
document.addEventListener('DOMContentLoaded', function () {
  var toggleActive = function (e, toggle) {
    e.stopPropagation();
    e.preventDefault();
    toggle ? e.target.classList.add('wavesurfer-dragover') :
      e.target.classList.remove('wavesurfer-dragover');
  };

  var handlers = {
    // Drop event
    drop: function (e) {
      toggleActive(e, false);

      // Load the file into wavesurfer
      if (e.dataTransfer.files.length) {
        wavesurfer.loadBlob(e.dataTransfer.files[0]);
      } else {
        wavesurfer.fireEvent('error', 'Not a file');
      }
    },

    // Drag-over event
    dragover: function (e) {
      toggleActive(e, true);
    },

    // Drag-leave event
    dragleave: function (e) {
      toggleActive(e, false);
    }
  };

  var dropTarget = document.querySelector('#drop');
  Object.keys(handlers).forEach(function (event) {
    dropTarget.addEventListener(event, handlers[event]);
  });
});

/*
 *  Time plugin to show current second and whole song length
 */
function TimePlugin() {
  $=jQuery;
  function secondsTimeSpanToHMS(s) {
    var m = Math.floor(s/60); //Get remaining minutes
    s -= m*60;
    return (m < 10 ? '0'+m : m)+":"+(s < 10 ? '0'+ s.toFixed(2) : s.toFixed(2)); //zero padding on minutes and seconds
  }

  var duration = wavesurfer.backend.getDuration();
  duration = secondsTimeSpanToHMS(duration);

  var counter = 0;

  $(".node__title").append(' <div class="seconds">' + '<span class="counter">' + counter + "</span> / " + duration + ' </div>');

  var myInterval = setInterval(function () {
    var counter = wavesurfer.backend.getCurrentTime();
    counter = secondsTimeSpanToHMS(counter);
    $(".counter").text(counter);
  }, 100);
}
