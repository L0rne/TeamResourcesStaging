<?php

/**
 *  Fetches a raw dataset of team responses to a diagnostic
 *
 */

class DiagnosticDMO {
    private $team_name = 'TEAM NAME NOT SET!';
    private $data = array();
    private $comments = array();
    public static $MAX = 65536; // see the logic in $this->get_nth_ranked_category. Assume we will never have this many categories.
    public static $MIN = 0;
    
    function __construct($line_item_id, $team = 'TEAM NAME NOT SET') {
        $this->team_name = $team;
        $this->data_fetch($line_item_id);
    }
    
    function __destruct() { $this->data = null; }
    
    /**
     *  Gets structured data based on arguments for:
     *  - filter: by category
     *  - xtab: by category or item
     *          crosstab data requires row, col, and data fn args
     *          (optional) column and row summaries
     *  - categories: returned ordered category array
     *
     *  Examples:
     *  args = array(
     *      
     *  );
     *
    */
    public function get_data($args = array()) {
        
        if (empty($args) || !isset($args['data_key'])) {
            return $this->data;
        }
        
        $key = $args['data_key'];
        switch ($key) {
            case 'team_name':
                return $this->team_name;
                break; // yeah, I know...but just in case.
            case 'member_comments':
                $args['grouping'] = 'id';
                $args['fn'] = 'mean';
                return $this->get_comments($args);
                break;
            case 'max_team_mean_category':
                $row = $this->get_xtab_rows(array('row' => 'team', 'col' => 'category', 'fn' => 'mean'));
                $max_category = $this->get_nth_ranked_category($row[0], self::$MAX);
                return array($max_category);
                break;
            case 'min_team_mean_category':
                $row = $this->get_xtab_rows(array('row' => 'team', 'col' => 'category', 'fn' => 'mean'));
                $min_category = $this->get_nth_ranked_category($row[0], self::$MIN);
                return array($min_category);
                break;
            case 'mean_of_category_sums': //REFACTOR +++
                return $this->get_mean_of_category_sums($args);
                break;
            default:
                break;
        }
        
        
        
    }
    
    public function get_mean_of_category_sums($args) {
        $data_args = array(
            'row' => 'team',
            'row_index' => 1,
            'col' => 'q_num',
            'fn' => 'mean',
            'category_filter' => $args['category_filter'],
            'row_label' => t('Team Avg'), //array('totals' => 'Total'), 
            'row_summary' => array('response_sum_mean'), //, 'mean') //REFACTORED to eliminate extra column!
        );
        //krumo($footer_args);
        $row = $this->get_xtab_rows($data_args);
        $row = $this->format_value_precision($row);
        
        return $row[1]['row_sum_response_sum_mean'];
    }
    
    public function get_nth_ranked_category($category_values = array(), $nth) {
        
        asort($category_values, SORT_NUMERIC);
        
        $last_index = count($category_values) - 1;
        $nth = $nth < 0 ? $nth = 0 : $nth;
        $nth = $nth > $last_index ? $last_index : $nth;
        $categories = array_keys($category_values);
        
        return $categories[$nth];
        
    }
    
    /**
     *  This function returns the Diagnostic dataset categories in the order they
     *  were fetched from the database, which is the same order they will be in
     *  for most of the series and table arrays.
     *
    */
    public function get_categories() {
        return $this->data['categories'];
    }
    
    /**
     *  This function puts the category array in the order requested by the client
     *  This allows the rest of the methods to function as is without the client
     *  having to manipulate the results after retrieving them.
     */
    public function set_category_order($order_csv) {
        
        $new_order = explode(',', $order_csv);
        
        //if (count($new_order) != count($this->data['categories'])) {
        //    throw new Exception('Report tried to re-order the data categories but the category list contained too many or too few. Check the Default Category Order in the Report Header content type. Then check the block in the report template PDF.');
        //}
        
        // check that all categories sent by client are in the data object's category array
        foreach ($new_order as $category) {
            if (!in_array($category, $this->data['categories'])) {
                $category_list = implode(',', $this->data['categories']);
                $category_list = preg_replace('/,/', ', ', $category_list);
                
                //die(var_dump($this->data['categories']));
                throw new Exception("The client code tried to set {$category} as a category. The report categories are {$category_list}. Check the content type and report template to ensure all of these categories are enumerated and spelled properly.");
            }
        }
        
        // check that all categories in teh data object's array are in the client list
        foreach ($this->data['categories'] as $category) {
            if (!in_array($category, $new_order)) {
                $order_csv = preg_replace('/,/', ', ', $order_csv);
                throw new Exception("The category order ({$order_csv}) sent by the client code does not contain {$category}. Check the content type and report template to ensure all of these categories are enumerated and spelled properly.");
            }
        }
        
        // the two arrays match item for item, so reset to the client's order of items.
        $this->data['categories'] = $new_order;
        
    }
    
    /**
     *  This function will re-order the results of other functions on the assumption that the dataset is
     *  an associative array that contains the categories in the array keys.
     *
     *  It also assumes that the client has already called the above function to set the default category
     *  order.
     *
     *  TODO: Expand this functionality to allow prefixed and suffixed items in the dataset (non-category items)
     *  to retain their positions and let the categories be the meat of the array sandwich.
     *  
     */
    public function order_dataset_categories($dataset) {
        
        $original_dataset = $dataset;
        // collect category elements from array.
        // remove them and capture their insert point.
        $category_index = -1;
        $dataset_index = 0;
        $dataset_prefix = array();
        $dataset_categories = array();
        $dataset_suffix = array();
        foreach ($dataset as $column => $data_value) {
            if (in_array($column, $this->data['categories'])) {
                $dataset_categories[$column] = $data_value;
                
                if ($category_index == -1) { $category_index = $dataset_index; }
            }
            else {
                if($category_index == -1) {
                    $dataset_prefix[$column] = $data_value;
                }
                else {
                    $dataset_suffix[$column] = $data_value;
                }
                
            }
            unset($dataset[$column]);
            $dataset_index++;
        }
        
        // re-order the categories
        $ordered_categories = array();
        foreach ($this->data['categories'] as $category) {
            if (array_key_exists($category, $dataset_categories)) {
                $ordered_categories[$category] = $dataset_categories[$category];
            }
        }
        
        // splice the categories back in
        $dataset = array_merge($dataset_prefix, $ordered_categories, $dataset_suffix);
        //$dataset = $dataset_prefix + $ordered_categories; // + $dataset_suffix;
        //die(var_dump($dataset));

        // make sure the ordering process actually ordered something.
        // else, return the original dataset.
        return count($ordered_categories) > 0 ? $dataset : $original_dataset;
    }
    
    /**
     *  Forms structured arrays for use in presentation charts.
     *
     *  Assumed chart types for UI:
     *
     *  1. category_means (options filter: {category})
     *  2. category_response_details
     *  3. item_mean (requires item_id)
     *
    */
    public function get_data_series($args = array('data_key' => 'category_means')) {
        $data_series = array();
        
        // NEXT STEP: use switch statement on chart type keys
        $data_key = $args['data_key'];
        switch ($data_key) {
            case 'category_means':
                $category_series = $this->get_xtab_rows(array('row' => 'team', 'row_index' => 'category_mean', 'col' => 'category', 'fn' => 'mean'));
                $data_series = $category_series;
                break;
            case 'question_means':
                $data_args = array('row' => 'team', 'row_index' => 'question_mean', 'col' => 'q_num', 'fn' => 'mean');
                if (isset($args['category_filter'])) $data_args['category_filter'] = $args['category_filter'];
                
                $question_series = $this->get_xtab_rows($data_args);
                $data_series = $question_series;
                //die(var_dump($data_series));
                break;
            case 'response_paired_category_sums':
                
                // collect response points
                foreach ($this->data['items'] as $item) {
                    foreach ($item['responses'] as $response_id => $points) {
                        $data_series['response_pairs'][$item['category']][$response_id][] = $points;
                    }
                }
                
                // calculate response sums
                foreach ($data_series['response_pairs'] as $cat_key => $category) {
                    foreach ($category as $response_id => $responses) {
                        $data_series['response_pairs'][$cat_key][$response_id] = (float) array_sum($responses);
                    }
                }
                
                if (isset($args['x_transform']) && isset($args['x_series'])) {
                    $x_series_name = $args['x_series'];
                    $x_transform = explode(',', $args['x_transform']);
                    $data_series['response_pairs'][$x_series_name] = $this->transform_data($data_series['response_pairs'][$x_series_name], $x_transform);  
                }
                if (isset($args['y_transform']) && isset($args['y_series'])) {
                    $y_series_name = $args['y_series'];
                    $y_transform = explode(',', $args['y_transform']);
                    $data_series['response_pairs'][$y_series_name] = $this->transform_data($data_series['response_pairs'][$y_series_name], $y_transform);  
                    //die(var_dump(array('file' => __FILE__, 'fn' => __FUNCTION__, "data_series['response_pairs']['$y_series_name']" => $data_series['response_pairs'][$y_series_name])));
                }
                break;
            case 'paired_category_sums_team_average';
                $categories = $this->get_categories();
                $response_count = 0;
                foreach ($categories as $category) {
                    $data_series['team_mean_pair'][$category] = array();
                }
                foreach ($this->data['items'] as $item) {
                    $category = $item['category'];
                    $response_count = $response_count < count($item['responses']) ? count($item['responses']) : $response_count;
                    $data_series['team_mean_pair'][$category] = array_merge($data_series['team_mean_pair'][$category], $item['responses']);
                }
                foreach ($data_series['team_mean_pair'] as $cat_key => $points) {
                    $data_series['team_mean_pair'][$cat_key] = (float) array_sum($points) / $response_count;
                }
                
                
                //die(var_dump(array('file' => __FILE__, 'fn' => __FUNCTION__,
                //                   "data_series" => $data_series
                //                   )
                //             )
                //    );
                
                if (isset($args['x_transform']) && isset($args['x_series'])) {
                    $x_series_name = $args['x_series'];
                    $x_transform = explode(',', $args['x_transform']);
                    $x_average = (int) $data_series['team_mean_pair'][$x_series_name];
                    $data_series['team_mean_pair'][$x_series_name] = (float)$x_transform[$x_average]; //$this->transform_data($data_series['team_mean_pair'][$x_series_name], $x_transform);
                }
                if (isset($args['y_transform']) && isset($args['y_series'])) {
                    $y_series_name = $args['y_series'];
                    $y_transform = explode(',', $args['y_transform']);
                    $y_average = (int) $data_series['team_mean_pair'][$y_series_name];
                    $data_series['team_mean_pair'][$y_series_name] = (float)$y_transform[$y_average]; // $this->transform_data($data_series['team_mean_pair'][$y_series_name], $y_transform);  
                    //die(var_dump(array('file' => __FILE__, 'fn' => __FUNCTION__, "data_series['response_pairs']['$y_series_name']" => $data_series['response_pairs'][$y_series_name])));
                }
                
                break;
            case 'category_response_details':
                break;
            case 'item_mean':
                break;
        }
        
        return $data_series;
        
    }
    
    protected function transform_data($series, $transform_array) {
        
        foreach ($series as $key => $value) {  
            $series[$key] = (float)$transform_array[$value];
        }
        
        //die(var_dump($series));
        return $series;
    }
    
    /**
     *  Forms structured arrays for use in presentation tables.
     *
     *  Assumed table types for UI:
     *
     *  1. member_mean_by_category:
     *     Response average (mean) by category with team category mean footer
     *  2. item_response_with_sum_and_mean{filter:category_name}
     *     Question item responses with sum and mean column summaries with
     *     ...Team question item mean footer AND team sum and mean column summaries
     *     ...Ranking of team question item means
     *  3. item_text{filter:category_name}
     *
     *
     *  case 'member_mean_by_category':
     *  case 'team_mean_by_category':
     *  case 'item_response_with_sum_and_mean':
     *  case 'member_points_by_question_with_sum_and_mean':
     *  case 'team_points_by_question_with_sum_and_mean':
     *  case 'numbered_items_filtered':
     *  case 'category_question_responses_with_row_and_column_summaries':
     *      
     *
    */
    public function get_table($args = array()) {
        $table = array(
            'header' => array(),
            'rows'   => array(),
            'footer' => array(),
        );
        
        
        $data_key = $args['data_key'];
        switch ($data_key) { //TODO: convert this switch into an array using the data_keys as keys to the table $args arrays
            case 'member_mean_by_category':
                $table['header'][0]['row_label'] = t('Member');
                foreach ($this->data['categories'] as $category) {
                    $table['header'][0][$category] = $category;
                }
                $rows = $this->get_xtab_rows(array('row' => 'responses', 'col' => 'category', 'fn' => 'mean'));
                //die(var_dump($rows));
                $table['rows'] = $this->format_value_precision($rows);
                break;
            case 'team_mean_by_category':
                break;
            case 'item_response_with_sum_and_mean':
                $row_args = array(
                    'row' => 'responses',
                    'col' => 'q_num',
                    'fn' => 'none',
                    'category_filter' => $args['category_filter'],
                    'row_summary' => array('sum','mean')
                );
                $rows = $this->get_xtab_rows($row_args);
                $table['rows'] = $this->format_value_precision($rows);
                break;
            case 'item_response_with_mean':
                $row_args = array(
                    'row' => 'responses',
                    'col' => 'q_num',
                    'fn' => 'none',
                    'category_filter' => $args['category_filter'],
                    'row_summary' => array('mean')
                );
                $rows = $this->get_xtab_rows($row_args);
                $table['rows'] = $this->format_value_precision($rows);
                if (!isset($args['footer_keys'])) {
                    $args['footer_keys'][] = 'team_mean_by_filtered_questions';
                }
                break;
            case 'member_points_by_question_with_sum_and_mean':
                $row_args = array(
                    'row' => 'responses',
                    'col' => 'q_num',
                    'fn' => 'none',
                    'category_filter' => $args['category_filter'],
                    'row_summary' => array('sum','mean')
                );
                $rows = $this->get_xtab_rows($row_args);
                $table['rows'] = $this->format_value_precision($rows);
                break;
            case 'team_points_by_question_with_sum_and_mean':
                $row_args = array(
                    'row' => 'team',
                    'col' => 'q_num',
                    'fn' => 'mean',
                    'category_filter' => $args['category_filter'],
                    'row_label' => array('totals' => 'Total'), 
                    'row_summary' => array('sum','mean')
                );
                $rows = $this->get_xtab_rows($row_args);
                $table['rows'] = $this->format_value_precision($rows);
                break;
            case 'numbered_items_filtered':
                //die(var_dump($args));
                $q_rows = array();
                //TODO: throw an error if category_filter is not set.
                $category_filter = $args['category_filter'];
                foreach ($this->data['items'] as $item) {
                    if (strtolower($item['category']) != strtolower($category_filter)) continue;
                    
                    $q_rows[] = array(
                        $item['q_num'] . '.',
                        $item['text'],
                    );
                }
                $table['rows'] = $q_rows;
                break;
            case 'category_question_responses_with_row_and_column_summaries':
                $row_args = array(
                    'row' => 'responses',
                    'col' => 'q_num',
                    'fn' => 'none',
                    'category_filter' => $args['category_filter'],
                    'row_summary' => array('sum','mean')
                );
                $rows = $this->get_xtab_rows($row_args);
                $table['rows'] = $this->format_value_precision($rows);
                //$args['footer_keys'][] = 'team_category_questions_response_with_summaries';
                //$args['footer_keys'][] = 'rank_team_mean_by_filtered_questions';
                break;
        }
        
        if (!isset($args['footer_keys'])) return $table;
        
        foreach ($args['footer_keys'] as $delta => $footer_key) {
            switch ($footer_key) {
                case 'total_mean_by_category':
                case 'team_mean_by_category':
                    $row = $this->get_xtab_rows(array('row' => 'team', 'row_index' => $delta, 'col' => 'category', 'fn' => 'mean'));
                    $row = $this->format_value_precision($row);
                    $footer_row = array_reverse($row[$delta], true);
                    $footer_row['row_label'] = t('Team Avg');
                    $table['footer'][$delta] = array_reverse($footer_row, true);
                    break;
                case 'rank_team_mean_by_category':
                    $row = $this->get_xtab_rows(array('row' => 'team', 'row_index' => $delta, 'col' => 'category', 'fn' => 'mean'));
                    $row = $this->format_value_precision($row);
                    $rank_row = $this->rank_cells($row[$delta]);
                    $footer_row = array_reverse($rank_row, true);
                    $footer_row['row_label'] = t('Rank');
                    $table['footer'][$delta] = array_reverse($footer_row, true);
                    break;
                case 'rank_team_mean_by_filtered_questions':
                    $footer_args = array(
                        'row' => 'team',
                        'row_index' => $delta,
                        'col' => 'q_num',
                        'fn' => 'mean',
                        'category_filter' => $args['category_filter'],
                    );
                    $row = $this->get_xtab_rows($footer_args);
                    $rank_row = $this->rank_cells($row[$delta]);
                    $footer_row = array_reverse($rank_row, true);
                    $footer_row['row_label'] = t('Rank');
                    $table['footer'][$delta] = array_reverse($footer_row, true);
                    break;
                case 'team_mean_by_filtered_questions':
                    $footer_args = array(
                        'row' => 'team',
                        'row_index' => $delta,
                        'col' => 'q_num',
                        'fn' => 'mean',
                        'category_filter' => $args['category_filter'],
                        'row_summary' => array('mean'),
                    );
                    $row = $this->get_xtab_rows($footer_args);
                    $row = $this->format_value_precision($row);
                    $footer_row = array_reverse($row[$delta], true);
                    $footer_row['row_label'] = t('Team Avg');
                    $table['footer'][$delta] = array_reverse($footer_row, true);
                    break;
                
                case 'team_category_questions_response_with_summaries':
                    $footer_args = array(
                        'row' => 'team',
                        'row_index' => $delta,
                        'col' => 'q_num',
                        'fn' => 'mean',
                        'category_filter' => $args['category_filter'],
                        'row_label' => t('Team Avg'), //array('totals' => 'Total'), 
                        'row_summary' => array('response_sum_mean'), //, 'mean')
                    );
                    $row = $this->get_xtab_rows($footer_args);
                    $row = $this->format_value_precision($row);
                    $footer_row = array_reverse($row[$delta], true);
                    $footer_row['row_label'] = t('Team Avg');
                    $table['footer'][$footer_key] = array_reverse($footer_row, true);
                    break;
                    
            }
        }
        
        //die(var_dump($table));
        return $table;
    }
    
    private function format_value_precision($numeric_data = array(), $precision = 2) {
        foreach ($numeric_data as $row_index => $row) {
            foreach ($row as $category => $value) {
                if (is_numeric($value))
                    $numeric_data[$row_index][$category] = number_format($value, $precision);
            }
        }
        return $numeric_data;
    }
    
    /**
     *  Returns an array of comments grouped by item or category with grouping text
     *  and aggregate calculation passed by client.
     *
     *  (optional arg) category_filter
     *  
     *  TODO: Implement group by category and calculate sum logic.
     *
    */
    public function get_comments($args = array('grouping' => 'id', 'fn' => 'mean')) {
        $groups = array();
        foreach ($this->data['items'] as $item) {
            
            if (isset($args['category_filter'])) {
                $filter = strtolower($args['category_filter']);
                $category = strtolower($item['category']);
                if ($filter != $category )continue;
            }
            
            $points = $item['responses'];
            $sum = array_sum($points);
            $count = count($points);
            $total = $count > 0 ? ($sum / $count) : 0;
            
            //$group_by = $args['grouping']; // for now, just group by item id. Need to implement aggregate calcs after foreach loop.
            
            $groups[$item['id']] = array(
                'text' => $item['q_num'] . '. ' . $item['text'],
                'category' => $item['category'],
                'total' => $total,
                'comments' => isset($this->data['comments'][$item['id']]) ? $this->data['comments'][$item['id']] : '',
            );
        }
        
        $groups = $this->order_comment_group($groups, $args);
        //die(var_dump($groups));
        
        return $groups;
    }
    
    protected function order_comment_group($groups, $args) {
        
        $allowed_fields = array('q_num','text','total');
        if (isset($args['order_by']) && in_array($args['order_by'], $allowed_fields)) {
            $id_values = array();
            foreach ($groups as $item_id => $item) {
                $id_values[$item_id] = $item[$args['order_by']];
            }
            
            $sort_type = $args['order_by'] == 'total' ? SORT_NUMERIC : SORT_STRING;
            if (asort($id_values, $sort_type)) {
                $ordered_ids = array_reverse($id_values, TRUE);
                $ordered_group = array();
                foreach ($ordered_ids as $id => $value_used) {
                    $ordered_group[$id] = $groups[$id];
                }
                $groups = $ordered_group;
            }
        }
        
        return $groups;
    }
    
    protected function rank_cells($cells = array(), $highest_to_lowest = true) {
        
        $sorted = $cells;
        asort($sorted, SORT_NUMERIC);
        $rank = $highest_to_lowest ? count($sorted) : 1;
        foreach ($sorted as $key => $cell_rank) {
            $cells[$key] = $highest_to_lowest ? $rank-- : $rank++;
        }
        
        return $cells; 
    }
    
    /**
     *  Forms a crosstab array for tables
     *
     *  $args = array(
     *      'row' => array_key, // team|responses|category|text
     *      'col' => array_key, // responses|category|text
     *      'fn'  => ['none'|'mean'|'sum'],
     *      'row_label' => string, // if not set, the row field name will be used
     *      'col_summary' => array($col_summ_header => ['mean'|'sum'], ...),
     *      'row_summary' => array($row_summ_header => ['mean'|'sum'|'rank'], ...),
     *      
     *  );
     *
     *  $output = array(
     *      'headers' => array(0 => $headers, ...), // each item in this array is a header row array
     *      'rows' => array(...),
     *      'footers' => array(0 => $footers, ...), // each item in this array is a footer row array
     *  );
     */
    protected function get_xtab_rows($args = array()) {
        if (empty($args)) {
            $args = array(
                'row' => 'team', 
                'col' => 'category',
                'fn'  => 'mean',
            //   'category_filter' => 'Clear Roles',
            //    'row_label' => array('member_num' => 'Member'), 
            //    'row_summary' => array('mean', 'sum'),
            );
        }
        
        // gather data points into an array for each column.
        $row_idx = isset($args['row_index']) ? $args['row_index'] : 0;
        $rows = array();
        $data_col_names = array();
        $response_count = 0;
        foreach ($this->data['items'] as $item) {
            
            if (isset($args['category_filter']) && strtolower($item['category']) != strtolower($args['category_filter'])) continue;
            
            $col = $item[$args['col']];
            if (!in_array($col, $data_col_names)) $data_col_names[] = $col;
            
            $response_count = count($item['responses']) > $response_count ? count($item['responses']) : $response_count;
            
            foreach ($item['responses'] as $member_id => $points) {
                
                if ($args['row'] == 'responses') { // this simple switch assumes we're either summarizing by member or by team as a whole.
                    $row_idx = $member_id;
                    $rows[$row_idx]['row_label'] = '#' . $member_id;
                }
                $rows[$row_idx][$col][] = $points;
            }
            
        }
        
        // sort the rows by $row_idx
        ksort($rows);
        
        // calculate the column function
        foreach ($rows as $key => $row) {
            $row_summary = array();
            foreach ($data_col_names as $data_col) {
                // first, collect data point for row summaries
                if (!empty($rows[$key][$data_col])) {
                    $row_summary = array_merge($row_summary, $rows[$key][$data_col]);
                }
                
                // now, calculate the column function
                $sum = null;
                if (isset($rows[$key][$data_col])) {
                    $sum = array_sum($rows[$key][$data_col]);
                }
                
                if ($args['fn'] == 'mean') {
                    $count = count($rows[$key][$data_col]);
                    $column_data = $count > 0 ? (float)$sum / $count : null;
                    $rows[$key][$data_col] = $sum / $count; //number_format($sum / $count, 2); //assume fixed precision of 2 decimal places for means. number_format(round($column_data, 2), 2);
                }
                else {
                    $rows[$key][$data_col] = $sum;
                }
            }
            
            if (!isset($args['row_summary'])) continue;
            
            // if there are row summaries, perform those functions here...
            $row_sum = array_sum($row_summary);
            foreach ($args['row_summary'] as $summary) {
                if ($summary == 'mean') {
                    $count = count($row_summary);
                    //$column_data = $count > 0 ? (float)$row_sum / $count : null;
                    $rows[$key]['row_sum_' . $summary] = $row_sum / $count; //number_format($row_sum / $count, 2); //assume fixed precision of 2 decimal places for means. number_format(round($column_data, 2)); 
                }
                elseif ($summary == 'response_sum_mean') {
                    $rows[$key]['row_sum_' . $summary] = $row_sum / $response_count; //number_format($row_sum / $response_count, 2);
                }
                else {
                    $rows[$key]['row_sum_' . $summary] = $row_sum;
                }
            }
        }
        
        //die(var_dump($rows));
        foreach ($rows as $key => $data_row) {
            $rows[$key] = $this->order_dataset_categories($data_row);
        }
        return $rows;
    }
    
    
    /**
     *  Fetches the raw response data from the database and
     *  structures it into a relational associative array:
     *
     *  $data = array(
     *      'team' => array(response_id => string team_member_email, ...),
     *      'categories' => array(category, ...),
     *      'items' => array(
     *          delta => array(
     *             'id' => int id,
     *             'category' => string category,
     *             'text' => string item_text,
     *             'type' => string item_type,
     *             'responses' => array(
     *                  response_id => float points,
     *                  ...(more responses)
     *             ),
     *          ),
     *          ...(more items)
     *      ),
     *  );
     *
     */
    protected function data_fetch($line_item_id) {
        // joins
        $query = db_select('tptd_response_details', 'd');
        $query->innerJoin('tptd_survey_items', 'i', 'd.item_id = i.item_id');
        $query->innerJoin('tptd_response', 'r', 'd.response_id = r.response_id');
        // fields
        $query->addField('r', 'response_id');
        $query->addField('r', 'team_member_email');
        $query->addField('i', 'item_id');
        $query->addField('i', 'item_type');
        $query->addField('i', 'item_text');
        $query->addField('i', 'category');
        $query->addField('i', 'delta');
        $query->addField('d', 'points');
        $query->addField('d', 'comments');
        // conditions
        $query->condition('r.order_line_item_id', $line_item_id, '=');
        //$query->condition('r.response_id', array(563,552,556,558,550,553,584), 'NOT IN');
        //$query->condition('d.points', 0, '>');
        $query->orderBy('i.delta', 'ASC');
        $query->orderBy('r.response_id', 'ASC');
        
        $result = $query->execute();
        
        $categories = array();
        $items = array();
        $responses = array();
        $participants = array();
        $comments = array();
        
        $member_numbers = array();
        $member_n = 1;
        while ($record = $result->fetchAssoc()) {
          // structure by items
          if (!in_array($record['category'], $categories)) {
            $categories[] = $record['category'];
          } //TODO: implement "report order" for categories
          
          if (!array_key_exists($record['response_id'], $member_numbers)) {
            $member_numbers[$record['response_id']] = $member_n++;
          }
          $this_member = $member_numbers[$record['response_id']];
          
          $participants[$this_member] = $record['team_member_email'];
          
          if (strlen($record['comments']) > 1) {
            $comments[$record['item_id']][$this_member] = $record['comments'];
          }
          
          if ((float)$record['points'] == 0) continue; // don't include zero-answers. They are non-answers in this context.
          
          $responses[$record['delta']][$this_member] = (float)$record['points'];
          $items[$record['delta']] = array(
            'id'        => (int)$record['item_id'],
            'category'  => $record['category'],
            'q_num'     => 'Q' . ($record['delta'] + 1),
            'text'      => $record['item_text'],
            'type'      => $record['item_type'],
            'responses' => $responses[$record['delta']],
          );
        }
        
        $this->data = array(
            'team' => $participants,
            'categories' => $categories,
            'items' => $items,
            'comments' => $comments,
        );
        
        //for testing...
        //$data_to_file = serialize($this->data);
        //file_put_contents('/Users/marcporlier/Sites/php-sandbox/data.inc', $data_to_file);
        
    }
}
