<div>Please enter your desired username, email address and password below to register.</div>
<div>
		<div class="registration-username-section">
        	<?php print $account_name; // (username field)?>
        </div>
        <div class="registration-email-confirmation-section"> 
            <?php print $account_mail; // (E-mail address field) ?>
            <?php print $confirm_email; // (Verify email field) ?>
        </div>
        <div class="registration-passwords-section">
            <?php print $account_password; // (Password and Verify Password fields ?>
        </div>
        <div class="registration-submit-actions">
            <?php print $form_submit_action; ?>
        </div>
</div>