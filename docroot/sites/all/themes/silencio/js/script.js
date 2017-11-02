/**
 * @file
 * A JavaScript file for the theme.
 *
 * In order for this JavaScript to be loaded on pages, see the instructions in
 * the README.txt next to this file.
 */

// JavaScript should be made compatible with libraries other than jQuery by
// wrapping it with an "anonymous closure". See:
// - https://drupal.org/node/1446420
// - http://www.adequatelygood.com/2010/3/JavaScript-Module-Pattern-In-Depth
(function ($, Drupal, window, document, undefined) {


// To understand behaviors, see https://drupal.org/node/756722#behaviors
  Drupal.behaviors.my_custom_behavior = {
    attach: function(context, settings) {

      wavesurfer.on('ready', function () {

        $(".btn-flag-1").click(function(){
          var currentTime =  wavesurfer.backend.getCurrentTime();
          $("#edit-field-time-und-0-value").val(currentTime);
          $(window).scrollTop($('#edit-field-time-und-0-value').position().top);
        });

        var id = 1;

        $( ".field-name-field-time .field-items" ).each(function() {
          id ++;
          var markerPos = $(this).text();
          wavesurfer.mark({
            id: id,
            color: 'rgba(0, 255, 0, 0.5)',
            position: markerPos
          });
        });

      });

    }
  };

})(jQuery, Drupal, this, this.document);
