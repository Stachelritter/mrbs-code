<?php
declare(strict_types=1);
namespace MRBS;

use chillerlan\QRCode\QRCode;
use chillerlan\QRCode\QROptions;
use MRBS\Form\ElementInputDate;
use MRBS\Form\ElementInputSearch;
use MRBS\Form\ElementInputSubmit;
use MRBS\Form\Form;


function print_head(bool $simple=false) : void
{
  global $refresh_rate;

  echo "<head>\n";

  echo "<meta charset=\"" . get_charset() . "\">\n";

  // Set IE=edge so that IE10 will display MRBS properly, even if compatibility mode is used
  // on the browser.  If we don't do this then MRBS will treat IE10 as an unsupported browser
  // when compatibility mode is turned on, potentially confusing users who may have forgotten
  // that they are using compatibility mode.   Unfortunately we can't set IE=edge in the header,
  // which is where we would normally do it, because then we won't be able to detect IE9 using
  // conditional comments.  So we have to do it in a <meta> tag, after the conditional comments
  // around the <html> tags.
  echo "<meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">\n";

  // Improve scaling on mobile devices
  echo "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n";

  if (!$simple)
  {
    // Add the CSRF token so that JavaScript can use it
    echo "<meta name=\"csrf_token\" content=\"" . escape_html(Form::getToken()) . "\">\n";
  }

  echo "<meta name=\"robots\" content=\"noindex, nofollow, noarchive\">\n";

  if (($refresh_rate != 0) && (this_page(false, '.php') == 'index'))
  {
    // If we're using JavaScript we'll do the refresh by getting a new
    // table using Ajax requests, which means we only have to download
    // the table not the whole page each time
    echo "<noscript>\n";
    echo "<meta http-equiv=\"Refresh\" content=\"$refresh_rate\">\n";
    echo "</noscript>\n";
  }

  echo "<title>" . get_vocab("mrbs") . "</title>\n";

  require_once MRBS_ROOT . "/style.inc";

  if (!$simple)
  {
    require_once MRBS_ROOT . "/js.inc";
  }

  echo "</head>\n";
}


// Print the basic site information.   This function is used for all headers, including
// the simple header, and so mustn't require any database access.
function print_header_site_info() : void
{
  global $mrbs_company,
         $mrbs_company_url,
         $mrbs_company_logo,
         $mrbs_company_more_info;

  // Company logo, with a link to the company
  if (!empty($mrbs_company_logo))
  {
    echo "<div class=\"logo\">\n";
    if (!empty($mrbs_company_url))
    {
      echo '<a href="' . escape_html($mrbs_company_url) . '">';
    }
    // Suppress error messages in case the logo is a URL, in which case getimagesize() can
    // fail for any number of reasons, eg (a) allow_url_fopen is not enabled in php.ini or
    // (b) "SSL operation failed with code 1. OpenSSL Error messages: error:1416F086:SSL
    // routines:tls_process_server_certificate:certificate verify failed".   As the image
    // size is not essential we'll just carry on.
    $logo_size = @getimagesize($mrbs_company_logo);
    echo '<img src="' . $mrbs_company_logo . '"';
    echo ' alt="' . escape_html($mrbs_company) . '"';
    if (is_array($logo_size))
    {
      echo ' ' . $logo_size[3];
    }
    echo '>';

    if (!empty($mrbs_company_url))
    {
      echo "</a>\n";
    }
    echo "</div>\n";
  }

  // Company name, any extra company info and MRBS
  echo "<div class=\"company\">\n";
  if (!empty($mrbs_company_url))
  {
    echo '<a href="' . escape_html($mrbs_company_url) . '">';
  }
  echo '<span>' . escape_html($mrbs_company) . '</span>';
  if (!empty($mrbs_company_url))
  {
    echo "</a>\n";
  }
  if (!empty($mrbs_company_more_info))
  {
    // Do not put $mrbs_company_more_info through escape_html() as it is
    // trusted and allowed to contain HTML.
    echo "<span class=\"company_more_info\">$mrbs_company_more_info</span>\n";
  }
  echo '<a href="' . escape_html(multisite('index.php')) . '">' . get_vocab('mrbs') . "</a>\n";
  echo "</div>\n";
}


function print_goto_date(array $context) : void
{
  global $multisite, $site;

  if (!checkAuthorised('index.php', true))
  {
    // Don't show the goto box if the user isn't allowed to view the calendar
    return;
  }

  $form = new Form();

  $form_id = 'form_nav';

  $form->setAttributes(array('id'     => $form_id,
                             'class'  => 'js_hidden',
                             'action' => multisite('index.php')))
       ->addHiddenInput('view', $context['view']);

  if (isset($context['area']))
  {
    $form->addHiddenInput('area', $context['area']);
  }

  if (isset($room))
  {
    $form->addHiddenInput('room', $context['room']);
  }

  if ($multisite && isset($site) && ($site !== ''))
  {
    $form->addHiddenInput('site', $site);
  }

  $input = new ElementInputDate();
  // Add the 'navigation' class so that the JavaScript knows it can use hidden days
  $input->setAttributes(array(
    'name'        => 'page_date',
    'value'       => format_iso_date($context['year'], $context['month'], $context['day']),
    'class'       => 'navigation',
    'aria-label'  => get_vocab('goto'),
    'required'    => true,
    'data-submit' => $form_id)
  );

  $form->addElement($input);

  $submit = new ElementInputSubmit();
  $submit->setAttribute('value', get_vocab('goto'));
  $form->addElement($submit);

  $form->render();
}


function print_outstanding(string $query) : void
{
  $mrbs_user = session()->getCurrentUser();

  if (!isset($mrbs_user))
  {
    return;
  }

  // Provide a link to the list of bookings awaiting approval
  // (if there are any enabled areas where we require bookings to be approved)
  $approval_somewhere = some_area('approval_enabled', TRUE);
  if ($approval_somewhere && ($mrbs_user->level > 0))
  {
    $n_outstanding = get_entries_n_outstanding($mrbs_user);

    $class = 'notification';

    if ($n_outstanding > 0)
    {
      $class .= ' attention';
    }

    $url = 'pending.php';
    if ($query !== '')
    {
      $url .= "?$query";
    }
    echo '<a href="' . escape_html(multisite($url)) . '"' .
         " class=\"$class\"" .
         ' title="' . get_vocab('outstanding', $n_outstanding) .
         "\">$n_outstanding</a>\n";
  }
}


function print_menu_items(string $query) : void
{
  global $auth, $kiosk_mode_enabled;

  $menu_items = array('help'   => 'help.php',
                      'report' => 'report.php',
                      'import' => 'import.php');

  if ($kiosk_mode_enabled)
  {
    $menu_items['kiosk'] = 'kiosk.php';
  }

  $menu_items['rooms'] = 'admin.php';

  if (auth()->canCreateUsers())
  {
    $menu_items['user_list'] = 'edit_users.php';
  }

  foreach ($menu_items as $token => $page)
  {
    // Only print menu items for which the user is allowed to access the page
    if (checkAuthorised($page, true))
    {
      $url = $page;
      if ($query !== '')
      {
        $url .= "?$query";
      }
      echo '<a href="' . escape_html(multisite($url)) . '">' . get_vocab($token) . "</a>\n";
    }
  }
}



function print_search(array $context) : void
{
  if (!checkAuthorised('search.php', true))
  {
    // Don't show the search box if the user isn't allowed to search
    return;
  }

  echo "<div>\n";

  $form = new Form(Form::METHOD_POST);

  $form->setAttributes(array(
          'id'     => 'header_search',
          'action' => multisite('search.php'))
        )
       ->addHiddenInputs(array(
          'view'       => $context['view'],
          'year'       => $context['year'],
          'month'      => $context['month'],
          'day'        => $context['day'],
          'from_date'  => format_iso_date($context['year'], $context['month'], $context['day'])
        )
     );

  if (!empty($context['area']))
  {
    $form->addHiddenInput('area', $context['area']);
  }
  if (!empty($context['room']))
  {
    $form->addHiddenInput('room', $context['room']);
  }

  $input = new ElementInputSearch();
  $search_vocab =  get_vocab('search');

  $input->setAttributes(array('name'        => 'search_str',
                              'placeholder' => $search_vocab,
                              'aria-label'  => $search_vocab,
                              'required'    => true));

  $form->addElement($input);

  $submit = new ElementInputSubmit();
  $submit->setAttributes(array('value' => get_vocab('search_button'),
                               'class' => 'js_none'));
  $form->addElement($submit);

  $form->render();

  echo "</div>\n";
}


// Generate the username link, which gives a report on the user's upcoming bookings.
function print_report_link(User $user) : void
{
  // If possible, provide a link to the Report page, otherwise the Search page
  // and if that's not possible just print the username with no link.  (Note that
  // the Search page isn't the perfect solution because it searches for any bookings
  // containing the search string, not just those created by the user.)
  if (checkAuthorised('report.php', true))
  {
    $attributes = array('action' => multisite('report.php'));
    $hidden_inputs = array('phase'        => '2',
                           'creatormatch' => $user->username);
  }
  elseif (checkAuthorised('search.php', true))
  {
    $attributes = array('action' => multisite('search.php'));
    $date_now = new DateTime();
    $hidden_inputs = array(
      'search_str' => $user->username,
      'from_date'  => $date_now->getISODate()
    );
  }
  else
  {
    echo '<span>' . escape_html($user->display_name) . '</span>';
    return;
  }

  // We're authorised for either Report or Search so print the form.
  $form = new Form(Form::METHOD_POST);

  $attributes['id'] = 'show_my_entries';
  $form->setAttributes($attributes)
       ->addHiddenInputs($hidden_inputs);

  $submit = new ElementInputSubmit();
  $submit->setAttributes(array('title' => get_vocab('show_my_entries'),
                               'value' => $user->display_name));
  $form->addElement($submit);

  $form->render();
}


function print_logonoff_button(array $params, string $value) : void
{
  $form = new Form($params['method']);
  $form->setAttributes(array('action' => $params['action']));

  // A Get method will replace the query string in the action URL with a query
  // string made up of the hidden inputs.  So put any parameters in the action
  // query string into hidden inputs.
  if ($params['method'] == Form::METHOD_GET)
  {
    $query_string = parse_url($params['action'], PHP_URL_QUERY);
    if (isset($query_string))
    {
      parse_str($query_string, $query_parameters);
      $form->addHiddenInputs($query_parameters);
    }
  }

  // Add the hidden fields
  if (isset($params['hidden_inputs']))
  {
    $form->addHiddenInputs($params['hidden_inputs']);
  }

  // The submit button
  $element = new ElementInputSubmit();
  $element->setAttribute('value', $value);
  $form->addElement($element);

  $form->render();
}


function print_logon() : void
{
  if (method_exists(session(), 'getLogonFormParams'))
  {
    $form_params = session()->getLogonFormParams();
    if (isset($form_params))
    {
      print_logonoff_button($form_params, get_vocab('login'));
    }
  }
}


function print_logoff() : void
{
  if (method_exists(session(), 'getLogoffFormParams'))
  {
    $form_params = session()->getLogoffFormParams();
    if (isset($form_params))
    {
      print_logonoff_button($form_params, get_vocab('logoff'));
    }
  }
}

// $context is an associative array indexed by 'view', 'view_all', 'year', 'month', 'day', 'area' and 'room'.
// When $omit_login is true the Login link is omitted.
function print_banner(?array $context, $simple=false, $omit_login=false) : void
{
  global $kiosk_QR_code, $auth;

  echo '<header class="banner' . (($simple) ? ' simple' : '') . "\">\n";

  $vars = array();

  if (isset($context['view']))
  {
    $vars['view'] = $context['view'];
  }
  if (isset($context['year']) && isset($context['month']) && isset($context['day']))
  {
    $vars['page_date'] = format_iso_date($context['year'], $context['month'], $context['day']);
  }
  if (isset($context['area']))
  {
    $vars['area'] = $context['area'];
  }
  if (isset($context['room']))
  {
    $vars['room'] = $context['room'];
  }

  $query = http_build_query($vars, '', '&');

  print_header_site_info();

  if (!$simple)
  {
    echo "<nav class=\"container\">\n";

    echo "<nav>\n";

    echo "<nav class=\"menu\">\n";
    print_menu_items($query);
    echo "</nav>\n";

    echo "<nav class=\"logon\">\n";
    print_outstanding($query);

    $mrbs_user = session()->getCurrentUser();

    // The empty string username is a special case when using anonymous booking
    if (isset($mrbs_user) && (!$auth['allow_anonymous_booking'] || ($mrbs_user->username !== '')))
    {
      print_report_link($mrbs_user);
      print_logoff();
    }
    elseif (!$omit_login)
    {
      print_logon();
    }

    echo "</nav>\n";

    echo "</nav>\n";

    echo "<nav>\n";
    print_goto_date($context);
    print_search($context);
    echo "</nav>\n";

    echo "</nav>\n";

    // Add in a QR code for kiosk mode
    // (The QR code library requires PHP 7.4 or greater and the mbstring extension)
    if (isset($context['kiosk']) &&
        $kiosk_QR_code &&
        (version_compare(PHP_VERSION, '7.4') >= 0) &&
        //Check for a Mbstring constant rather than using extension_loaded, which is sometimes disabled
        defined('MB_CASE_UPPER'))
    {
      $url = multisite(url_base() . "/index.php?$query");
      echo '<nav class="qr" title="' . escape_html($url) . "\">\n";
      $options = new QROptions([
        'outputType'  => QRCode::OUTPUT_MARKUP_SVG,
        'imageBase64' => false,
      ]);
      $qrcode = new QRCode($options);
      echo $qrcode->render($url);
      echo "</nav>\n";
    }
  }

  echo "</header>\n";
}



// Print a message which will only be displayed (thanks to CSS) if the user is
// using an unsupported browser.
function print_unsupported_message(?array $context) : void
{
  echo "<div class=\"unsupported_message\">\n";
  print_banner($context, true);
  echo "<div class=\"contents\">\n";
  echo "<p>" . get_vocab('browser_not_supported', get_vocab('mrbs_abbr')) . "</p>\n";
  echo "</div>\n";
  echo "</div>\n";
}


// Print the page header
// $context is an associative array indexed by 'view', 'view_all', 'year', 'month', 'day', 'area' and 'room',
// any of which can be NULL.
// If $simple is true, then just print a simple header that doesn't require any database
// access or JavaScript (useful for fatal errors and database upgrades).
// When $omit_login is true the Login link is omitted.
function print_theme_header(?array $context=null, bool $simple=false, bool $omit_login=false) : void
{
  global $multisite, $site, $default_view, $default_view_all, $view_week_number, $style_weekends;

  if ($simple)
  {
    $data = array();
    $classes = array();
  }
  else
  {
    // Set the context values if they haven't been given
    if (!isset($context))
    {
      $context = array();
    }

    if (empty($context['area']))
    {
      $context['area'] = get_default_area();
    }

    if (empty($context['room']))
    {
      $context['room'] = get_default_room($context['area']);
    }

    if (!isset($context['view']))
    {
      $context['view'] = (isset($default_view)) ? $default_view : 'day';
    }

    if (!isset($context['view_all']))
    {
      $context['view_all'] = (isset($default_view_all)) ? $default_view_all : true;
    }

    // Need to set the timezone before we can use date()
    // This will set the correct timezone for the area
    get_area_settings($context['area']);

    // If we don't know the right date then use today's
    if (!isset($context['year']))
    {
      $context['year'] = (int) date('Y');
    }

    if (!isset($context['month']))
    {
      $context['month'] = (int) date('m');
    }

    if (!isset($context['day']))
    {
      $context['day'] = (int) date('d');
    }

    // Get the form token now, before any headers are sent, in case we are using the 'cookie'
    // session scheme.  Otherwise we won't be able to store the Form token.
    Form::getToken();

    $page = this_page(false, '.php');

    // Put some data attributes in the body element for the benefit of JavaScript.  Note that we
    // cannot use these PHP variables directly in the JavaScript files as those files are cached.
    $data = array(
      'view'          => $context['view'],
      'view_all'      => $context['view_all'],
      'area'          => $context['area'],
      'room'          => $context['room'],
      'page'          => $page,
      'page-date'     => format_iso_date($context['year'], $context['month'], $context['day']),
      'is-admin'      => (is_admin()) ? 'true' : 'false',
      'is-book-admin' => (is_book_admin()) ? 'true' : 'false',
      'lang-prefs'    => json_encode(get_lang_preferences())
    );

    if ($multisite && isset($site) && ($site !== ''))
    {
      $data['site'] = $site;
    }

    if (isset($context['kiosk']))
    {
      $data['kiosk'] = $context['kiosk'];
    }

    $mrbs_user = session()->getCurrentUser();
    if (isset($mrbs_user))
    {
      $data['username'] = $mrbs_user->username;
    }

    // We need $timetohighlight for the day and week views
    $timetohighlight = get_form_var('timetohighlight', 'int');
    if (isset($timetohighlight))
    {
      $data['timetohighlight'] = $timetohighlight;
    }

    // Put the filename in as a class to aid styling.
    $classes[] = $page;
    // And if the user is logged in, add another class to aid styling
    if (isset($mrbs_user))
    {
      $classes[] = 'logged_in';
    }

    // To help styling
    if ($view_week_number)
    {
      $classes[] = 'view_week_number';
    }
    if ($style_weekends)
    {
      $classes[] = 'style_weekends';
    }

  }

  $headers = array("Content-Type: text/html; charset=" . get_charset());
  http_headers($headers);

  echo DOCTYPE . "\n";

  // We produce two <html> tags: one for versions of IE that we don't support and one for all
  // other browsers.  This enables us to use CSS to hide and show the appropriate text.
  echo "<!--[if lte IE 9]>\n";
  echo "<html lang=\"" . escape_html(get_mrbs_lang()) . "\" class=\"unsupported_browser\">\n";
  echo "<![endif]-->\n";
  echo "<!--[if (!IE)|(gt IE 9)]><!-->\n";
  echo "<html lang=\"" . escape_html(get_mrbs_lang()) . "\">\n";
  echo "<!--<![endif]-->\n";

  print_head($simple);

  echo '<body class="' . escape_html(implode(' ', $classes)) . '"';
  foreach ($data as $key => $value)
  {
    if (isset($value))
    {
      // Convert booleans to 0 or 1
      if (is_bool($value))
      {
        $value = (int)$value;
      }
      echo " data-$key=\"" . escape_html($value) . '"';
    }
  }
  echo ">\n";

  print_unsupported_message($context);

  print_banner($context, $simple, $omit_login);

  // This <div> should really be moved out of here so that we can always see
  // the matching closing </div>
  echo "<div class=\"contents\">\n";


} // end of print_theme_header()

