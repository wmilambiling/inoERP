<?php

/**
 * inoERP
 *
 * @copyright   2014 Nishit R. Das
 * @license     https://www.mozilla.org/MPL/2.0/
 * @link        http://inoideas.org
 * @source code https://github.com/inoerp/inoERP
 */

/**
 * fa_depreciation_header
 * Contains all the fa_depreciation_header information, such as - depreciation_header, method_type, description,  calcualtion_basis, etc.
 * 
 */
class fa_depreciation_header extends dbObject implements gl_journal_int {

 public static $table_name = "fa_depreciation_header";
 public static $primary_column = "fa_depreciation_header_id";
 public static $dependent_classes = ['fa_depreciation_line'];
 public static $key_column = "fa_asset_book_id";
 public static $module = "fa";
 public static $js_fileName_prg = 'modules/fa/depreciation/depreciation.js';
 public static $gl_journal_category = "FA_DEPRECIATION";
 public static $system_info = [
  'name' => 'depreciation',
  'number' => '1406',
  'description' => 'Depreciation ',
  'version' => '0.1.1',
  'db_version' => '1001',
  'mod_version' => '1.1.1',
  'primary_entity_cb' => '',
  'dependent_class' => array('fa_depreciation_line'),
  'primary_entity_cb' => '',
  'module_name' => 'fp',
  'weight' => 2
 ];
 public static $status_a = [
  'ENTERED' => 'Entered',
  'CONFIRMED' => 'Confirmed',
  'COMPLETED' => 'Completed',
  'CLOSED' => 'Closed',
 ];
 public $field_a = [
  'fa_depreciation_header_id',
  'fa_asset_book_id',
  'gl_period_id',
  'status',
  'description',
  'gl_journal_header_id',
  'created_by',
  'creation_date',
  'last_update_by',
  'last_update_date',
 ];
 public $fields_inForm_notInDataBase = [
  'action'
 ];
 public $notUpdatedFromWebForm = [
  'gl_journal_header_id',
 ];
 public $initial_search = [
  'fa_depreciation_header_id',
 ];
 public $requiredField = [
  'fa_asset_book_id',
  'gl_period_id',
 ];
 public $search = [
  '_show_update_path' => 1,
  '_show_view_path' => 1,
 ];
 public $pageTitle = " Depreciation "; //page Title
 public $fa_depreciation_header_id;
 public $fa_asset_book_id;
 public $gl_period_id;
 public $status;
 public $description;
 public $gl_journal_header_id;
 public $created_by;
 public $creation_date;
 public $last_update_by;
 public $last_update_date;
 public $action;
 private $_max_line_num;
 private $_acc_dep_ac = [];
 public $prg_run_depreciation_parameters = [
  'Depreciation Id' => 'search_depreciation_id',
  'Asset Book' => 'search_asset_book',
  'Period' => 'search_depreciation_period'
 ];
 public $prg_run_depreciation_details = [
  'name' => 'Run Depreciation',
  'description' => 'Run depreciation program',
 ];

 public function _before_save() {
  if (!empty($this->action) && ($this->action == 'POST_DEPRECIATION')) {
   $this->_post_depreciation();
   return 10;
  }
 }

 private function _post_depreciation() {
  $fa_dh = new fa_depreciation_header();
  $this->_acc_dep_ac = [];
  $fa_dh->findBy_id($this->fa_depreciation_header_id);
  if (empty($this->gl_journal_header_id)) {
   $this->_save_journal_header();
   $this->_max_line_num = 1;
  } else {
   $gjh = new gl_journal_header();
   $gjh->findBy_id($this->gl_journal_header_id);
   $max_line_num = $gjh->highest_line_num()->max_line_num;
   $this->_max_line_num = !empty($max_line_num) ? $max_line_num + 1 : 1;
  }

  if (!empty($this->gl_journal_header_id)) {
   $this->_save_journal_lines();
  }
 }

 public function _save_journal_header() {
  global $dbc;
  $gjh = new gl_journal_header;

  $fa_ab = new fa_asset_book();
  $fa_ab->findBy_id($this->fa_asset_book_id);
  $org_fin_details = org::find_financial_details_from_orgId($fa_ab->bu_org_id);

  $gjh->ledger_id = $org_fin_details->ledger_id;
  $gjh->status = 'ENTERED';
  $gjh->currency = $org_fin_details->currency_code;
  $gjh->period_id = $this->gl_period_id;
  $gjh->journal_source = self::$module;
  $gjh->journal_category = self::$gl_journal_category;
  $gjh->reference_type = 'table';
  $gjh->reference_key_name = self::$table_name;
  $gjh->reference_key_value = $this->fa_depreciation_header_id;
  $gjh->journal_name = $gjh->journal_category . '-' . $gjh->reference_key_value;
  $gjh->description = $gjh->journal_name . '-' . current_time();
  $gjh->balance_type = 'A';

  try {
   $gjh->save();
   $this->gl_journal_header_id = $gjh->gl_journal_header_id;
   echo "<br> Jounral Header Id #" . $this->gl_journal_header_id . ' is created';

   //need to create a new insatnce of fa_depreciation_header as the gl_journal_header_id is not allowed to updated from webform
   $fa_dh_jh = new fa_depreciation_header();
   $fa_dh_jh->findBy_id($this->fa_depreciation_header_id);
   $fa_dh_jh->gl_journal_header_id = $gjh->gl_journal_header_id;
   $fa_dh_jh->status = 'COMPLETED';
   $fa_dh_jh->save();
  } catch (Exception $e) {
   echo "<br> Failed Saving Jounral Header" . $e->getMessage();
   $dbc->rollback = true;
  }
 }

 public function _post_gl_process($fa_depriciation_header_id) {
  try {
   $this->findBy_id($fa_depriciation_header_id);
   $this->status = 'CLOSED';
   $this->save();
   return 1;
  } catch (Exception $e) {
   echo "<br>Updating depreciation header status failed.Error @ " . __FILE__ . ' @@ line ' . __LINE__ . $e->getMessage();
   return -99;
  }
 }

 public function _save_journal_lines() {
  global $dbc;
  $fadl = new fa_depreciation_line();
  $all_lines = $fadl->findBy_parentId($this->fa_depreciation_header_id);
  foreach ($all_lines as $lines) {
   if (empty($lines->depreciation_amount)) {
    continue;
   }
   $new_gjl = new gl_journal_line();
   $all_asset_book_accounts = fa_book_category_association::find_all_accounts_byAssetBookId($lines->asset_id, $this->fa_asset_book_id);
   if (empty($all_asset_book_accounts)) {
    echo "<br>No accumulated depreciation account found. Exiting save journal. Error @ " . __FILE__ . ' @@ ' . __LINE__;
    $dbc->rollback = true;
    return -90;
   }
   $acc_deprn_acc = $all_asset_book_accounts->accumulated_depreciation_ac_id;
   $new_gjl->gl_journal_header_id = $this->gl_journal_header_id;
   $new_gjl->reference_key_name = 'fa_depreciation_line';
   $new_gjl->reference_key_value = $lines->fa_depreciation_line_id;
   //check if line lready exists
   $jl_exists = $new_gjl->findBy_reference_keyName_keyValue();
   if ($jl_exists) {
    $new_gjl->gl_journal_line_id = $jl_exists->gl_journal_line_id;
    $new_gjl->line_num = $jl_exists->line_num;
    $new_gjl->status = $jl_exists->status;
   } else {
    $new_gjl->gl_journal_line_id = null;
    $new_gjl->line_num = $this->_max_line_num;
    $new_gjl->status = 'U';
    $this->_max_line_num++;
   }
   $new_gjl->code_combination_id = $lines->depreciation_account_id;
   $new_gjl->total_dr = $new_gjl->total_ac_dr = $lines->depreciation_amount;
   if (key_exists($acc_deprn_acc, $this->_acc_dep_ac)) {
    $this->_acc_dep_ac[$acc_deprn_acc] += $lines->depreciation_amount;
   } else {
    $this->_acc_dep_ac[$acc_deprn_acc] = $lines->depreciation_amount;
   }

   $new_gjl->description = 'Depriciation of line id #' . $lines->fa_depreciation_line_id;
   $new_gjl->reference_type = 'table';
   $new_gjl->save();
  }

//	save the accumulated depriciation
  if (!empty($this->_acc_dep_ac)) {
   foreach ($this->_acc_dep_ac as $acc_id => $amount) {
    $acc_dep_gjl = new gl_journal_line();
    $acc_dep_gjl->gl_journal_header_id = $this->gl_journal_header_id;
    $acc_dep_gjl->reference_key_name = 'fa_depreciation_header_id';
    $acc_dep_gjl->reference_key_value = $this->fa_depreciation_header_id . '|' . $acc_id;
    //check if line lready exists
    $jl_exists_acc_dep = $acc_dep_gjl->findBy_reference_keyName_keyValue();
    if ($jl_exists_acc_dep) {
     $acc_dep_gjl->gl_journal_line_id = $jl_exists_acc_dep->gl_journal_line_id;
     $acc_dep_gjl->line_num = $jl_exists_acc_dep->line_num;
    } else {
     $acc_dep_gjl->gl_journal_line_id = null;
     $acc_dep_gjl->line_num = $this->_max_line_num;
     $this->_max_line_num++;
    }
    $acc_dep_gjl->status = 'U';
    $acc_dep_gjl->code_combination_id = $acc_id;
    $acc_dep_gjl->total_ac_cr = $acc_dep_gjl->total_cr = $amount;
    $acc_dep_gjl->description = 'Acc Depriciation for fa_depreciation_header_id ' . $this->fa_depreciation_header_id . ' & acc_id ' . $acc_id;
    $acc_dep_gjl->reference_type = 'table';
    $acc_dep_gjl->save();
   }
  }
 }

 private function _calculate_depreciation_amount($asset_r) {
  /* This method calucates the depriciation amount as per month */
  //$asset_r is aset_book_info
  if (empty($asset_r->depriciation_start_date) || empty($asset_r->life_months)) {
   return false;
  }
  $dprn_amount = 0;
  $dprn_method = new fa_depreciation_method();
  $dprn_method->findBy_id($asset_r->fa_depreciation_method_id);
  $curr_cost = $asset_r->current_cost;
  $acc_dprn_amount = $asset_r->accumulated_depreciation;
  $nbv = $asset_r->current_cost - $acc_dprn_amount;
  if ($dprn_method->calculation_basis == 'NBV') {
   $amount_availabe_for_dprn = $nbv - $asset_r->salvage_value_amount;
  } else {
   $amount_availabe_for_dprn = $curr_cost - $asset_r->salvage_value_amount;
  }

  $start_date = new DateTime($asset_r->depriciation_start_date);
  $current_date = new DateTime();
  $date_diff = $current_date->diff($start_date);
  $months_for_dprn_completed = ($date_diff->y * 12 ) + $date_diff->m;
  $months_to_depriciate = $asset_r->life_months - $months_for_dprn_completed;

  switch ($dprn_method->method_type) {
   case 'SLD' :
    $dprn_amount = $amount_availabe_for_dprn / $months_to_depriciate;
//    echo "$dprn_amount = $amount_availabe_for_dprn / $months_to_depriciate";
    break;

   case 'RBD' :
    $rate = !empty($dprn_method->reducing_balance_rate) ? $dprn_method->reducing_balance_rate : 0;
    $dprn_amount = ($amount_availabe_for_dprn * $rate) / 100;
    break;

   case 'SYDD' :
    $total_years = ceil($asset_r->life_months / 12);
    $total_years_for_dprn = 0;
    for ($i = 1; $i < $total_years; $i++) {
     $total_years_for_dprn +=$i;
    }
    $curr_year = floor($date_diff->y);
    $year_no_of_dprn = $total_years - $curr_year;
    $dprn_amount = [($amount_availabe_for_dprn * $year_no_of_dprn) / $total_years_for_dprn] / 12;
    break;

   case 'UAD' :
    break;

   case 'RATE' :
    $rate = fa_depreciation_method_rate::find_by_headerId_year_period($asset_r->fa_depreciation_method_id, $date_diff->y + 1, $date_diff->m + 1);
    $dprn_amount = ($amount_availabe_for_dprn * $rate) / 100;
    break;
  }

  return $dprn_amount;
 }

 public function calucate_depriciation_amount(stdClass $fa_ab_i) {
  return $this->_calculate_depreciation_amount($fa_ab_i);
 }

 private function _create_depreciation_lines() {
  echo '<br> in  line ' . __LINE__;
  $fa_dh = new fa_depreciation_header();
  $fa_dh->findBy_id($this->fa_depreciation_header_id);

  /* 1. Find all assets assigned to the depreciation book from asset_book_info
   * 2. For each asset creat a fa_depreciation_line */

  $fa_ast_book_info_i = fa_asset_book_info::find_by_bookId($fa_dh->fa_asset_book_id);

  if (empty($fa_ast_book_info_i)) {
   return;
  }

  foreach ($fa_ast_book_info_i as $asset_r) {
   $fa_dprn_line = new fa_depreciation_line();

   $fa_dprn_line->fa_depreciation_header_id = $this->fa_depreciation_header_id;
   $fa_dprn_line->asset_id = $asset_r->fa_asset_id;
   $dprn_amount = $this->_calculate_depreciation_amount($asset_r);
   $fa_dprn_line->depreciation_amount = $fa_dprn_line->total_depreciation_amount = $dprn_amount;
   $fa_asset_assgn = fa_asset_assignment::find_by_parent_id($asset_r->fa_asset_id);
   if (!empty($fa_asset_assgn[0]->expense_ac_id)) {
    $fa_dprn_line->depreciation_account_id = $fa_asset_assgn[0]->expense_ac_id;
   } else {
    $fa_dprn_line->depreciation_account_id = '999';
   }
   $fa_dprn_line->cost_before_depreciation = $asset_r->current_cost;
   $fa_dprn_line->nbv_before_depreciation = $asset_r->current_cost - $asset_r->accumulated_depreciation;
   $fa_dprn_line->salvage_value_bd = $asset_r->salvage_value_amount;

   //update the accumulated_depreciation amount
   $fa_ab_info = new fa_asset_book_info();
   $fa_asset_book_info_i = fa_asset_book_info::find_by_assetId_bookId($asset_r->fa_asset_book_id, $asset_r->fa_asset_id);
   $fa_ab_info->fa_asset_book_info_id = $fa_asset_book_info_i->fa_asset_book_info_id;
   $fa_ab_info->accumulated_depreciation += $dprn_amount;
   $fa_ab_info->save();

   try {
    $fa_dprn_line->save();
   } catch (Exception $e) {
    echo "<br/>Error @ fa_depreciation_header @@ line " . __LINE__ . $e->getMessage();
   }
   unset($dprn_amount);
  }
 }

 public function prg_run_depreciation($seralized_parameters) {
  $parameters = unserialize($seralized_parameters);
  $this->message = '<br> Starting depreciation program';

  if (!empty($parameters['fa_depreciation_header_id'][0])) {
   $this->fa_depreciation_header_id = $parameters['fa_depreciation_header_id'][0];
   $this->message .= '<br> Entered Org Id is : ' . $this->fa_depreciation_header_id;
  } else {
   $this->message .= '<br> Error! : No fa_depreciation_header_id found @ fa_depreciation_header  ' . __LINE__;
   return $this->message;
  }


  if (!empty($this->fa_depreciation_header_id)) {
   $this->message .= "<br/>Using fa_depreciation_header_id $this->fa_depreciation_header_id ";
   try {
    $this->_create_depreciation_lines();
    $this->message .= "<br/>Depreciation program is Successfullycompleted for fa_depreciation_header_id " . $this->fa_depreciation_header_id;
   } catch (Exception $e) {
    $this->message .= "<br/>Depreciation program fialed for Item fa_depreciation_header_id " . $this->fa_depreciation_header_id . $e->getMessage();
   }

   return $this->message;
  }
 }

}

//end of path class
?>
