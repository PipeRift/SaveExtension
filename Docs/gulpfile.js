var gulp = require('gulp');
var exec = require("child_process").exec;
var runSequence = require('run-sequence');
var gulpGitbook = require('gulp-gitbook');

gulp.task('default', function() {
  // place code for your default task here
});

gulp.task('build', function(cb) {
  runSequence(
    'refresh-summary',
    'generate-book',
    function() {
      gulpGitbook(".", cb);
    });
});

gulp.task('build-light', function(cb) {
  runSequence(
    'refresh-summary',
  function() {
    gulpGitbook(".", cb);
  });
});

gulp.task('serve', function(cb) {
  runSequence(
    'refresh-summary',
  function() {
    gulpGitbook.serve(".", cb);
  });
});

gulp.task('refresh-summary', function(cb) {
  exec('node ./node_modules/gitbook-summary/bin/summary.js sm', function(error, stdout, stderr) {
    cb(error);
  });
});

gulp.task('generate-book', function(cb) {
  gulpGitbook.pdf(".", {outputDir: "./downloads/book.pdf"}, function(err) {
    if(err) { cb(err); return; }

    gulpGitbook.epub(".", {outputDir: "./downloads/book.epub"}, function(err) {
      if(err) { cb(err); return; }

      gulpGitbook.mobi(".", {outputDir: "./downloads/book.mobi"}, cb);
    });
  });
});
