<?php
// $Id$

/**
 *  @file
 *  Outputs the view.
 *
 */
?>

<div id="views-jqfx-cloud-carousel-<?php print $id; ?>" class="views-jqfx-cloud-carousel-container">

  <div id="views-jqfx-cloud-carousel-images-<?php print $id; ?>" class=<?php print $classes; ?>>
    <?php foreach ($images as $image): ?>
      <?php print $image ."\n"; ?>
    <?php endforeach; ?>
    <div id="cloud-carousel-title-text" class="cloud-carousel-title-text"></div>
    <div id="cloud-carousel-alt-text" class="cloud-carousel-alt-text"></div>
    <div id="cloud-carousel-left-but" class="cloud-carousel-left-but"></div>
    <div id="cloud-carousel-right-but" class="cloud-carousel-right-but"></div>
  </div>

</div>

