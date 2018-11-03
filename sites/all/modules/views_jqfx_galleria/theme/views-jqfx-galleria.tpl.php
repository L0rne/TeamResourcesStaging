<?php
// $Id: views-jqfx-galleria.tpl.php,v 1.1.2.2 2010/08/24 15:00:54 aaron Exp $

/**
 *  @file
 *  This will format a view to display several images in a Galleria style, for
 *  Views jQfx: Galleria.
 *
 * - $view: The view object.
 * - $options: Style options. See below.
 * - $rows: The output for the rows.
 * - $title: The title of this group of rows.  May be empty.
 * - $id: The unique counter for this view.
 */
?>

  <div id="views-jqfx-galleria-<?php print $id; ?>" class="views-jqfx-galleria">
    <?php if (!empty($title)) : ?>
      <h3><?php print $title; ?></h3>
    <?php endif; ?>

    <div id="views-jqfx-galleria-images-<?php print $id; ?>" class="<?php print $classes; ?>">
      <?php foreach ($rows as $row): ?>
       <?php print $row ."\n"; ?>
      <?php endforeach; ?>
    </div>

  </div>

