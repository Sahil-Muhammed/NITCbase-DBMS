#include "Frontend.h"

#include <cstring>
#include <iostream>

int Frontend::create_table(char relname[ATTR_SIZE], int no_attrs, char attributes[][ATTR_SIZE],
                           int type_attrs[]) {
  // Schema::createRel
  return Schema::createRel(relname, no_attrs, attributes, type_attrs);
  //return SUCCESS;
}

int Frontend::drop_table(char relname[ATTR_SIZE]) {
  // Schema::deleteRel
  return Schema::deleteRel(relname);
  //return SUCCESS;
}

int Frontend::open_table(char relname[ATTR_SIZE]) {
  // Schema::openRel
  return Schema::openRel(relname);
  //return SUCCESS;
}

int Frontend::close_table(char relname[ATTR_SIZE]) {
  // Schema::closeRel
  return Schema::closeRel(relname);
}

int Frontend::alter_table_rename(char relname_from[ATTR_SIZE], char relname_to[ATTR_SIZE]) {
  // Schema::renameRel
  return Schema::renameRel(relname_from, relname_to);
}

int Frontend::alter_table_rename_column(char relname[ATTR_SIZE], char attrname_from[ATTR_SIZE],
                                        char attrname_to[ATTR_SIZE]) {
  // Schema::renameAttr
  return Schema::renameAttr(relname, attrname_from, attrname_to);
}

int Frontend::create_index(char relname[ATTR_SIZE], char attrname[ATTR_SIZE]) {
  // Schema::createIndex
  return Schema::createIndex(relname, attrname);
  //return SUCCESS;
}

int Frontend::drop_index(char relname[ATTR_SIZE], char attrname[ATTR_SIZE]) {
  // Schema::dropIndex
  return Schema::dropIndex(relname, attrname);
  //return SUCCESS;
}

int Frontend::insert_into_table_values(char relname[ATTR_SIZE], int attr_count, char attr_values[][ATTR_SIZE]) {
  // Algebra::insert
  return Algebra::insert(relname, attr_count, attr_values);
}

int Frontend::select_from_table(char relname_source[ATTR_SIZE], char relname_target[ATTR_SIZE]) {
  // Algebra::project
  //return SUCCESS;
  return Algebra::project(relname_source, relname_target);
}

int Frontend::select_attrlist_from_table(char relname_source[ATTR_SIZE], char relname_target[ATTR_SIZE],
                                         int attr_count, char attr_list[][ATTR_SIZE]) {
  // Algebra::project
  //return SUCCESS;
  return Algebra::project(relname_source, relname_target, attr_count, attr_list);
}

int Frontend::select_from_table_where(char relname_source[ATTR_SIZE], char relname_target[ATTR_SIZE],
                                      char attribute[ATTR_SIZE], int op, char value[ATTR_SIZE]) {
  // Algebra::select
  return Algebra::select(relname_source, relname_target, attribute, op, value);
}

int Frontend::select_attrlist_from_table_where(char relname_source[ATTR_SIZE], char relname_target[ATTR_SIZE],
                                               int attr_count, char attr_list[][ATTR_SIZE],
                                               char attribute[ATTR_SIZE], int op, char value[ATTR_SIZE]) {
  // Algebra::select + Algebra::project??
  //return SUCCESS;
  int retValselect = Algebra::select(relname_source, (char *)TEMP, attribute, op, value);

  if (retValselect < 0){
    return retValselect;
  }

  int retValopenRel = OpenRelTable::openRel((char *)TEMP);

  if (retValopenRel < 0){
    Schema::deleteRel((char *)TEMP);
    return retValopenRel;
  }

  int retValproject = Algebra::project((char *)TEMP, relname_target, attr_count, attr_list);
  int tempRelId = OpenRelTable::getRelId((char *)TEMP);
  OpenRelTable::closeRel(tempRelId);
  Schema::deleteRel((char *)TEMP);

  return retValproject;
}

int Frontend::select_from_join_where(char relname_source_one[ATTR_SIZE], char relname_source_two[ATTR_SIZE],
                                     char relname_target[ATTR_SIZE],
                                     char join_attr_one[ATTR_SIZE], char join_attr_two[ATTR_SIZE]) {
  // Algebra::join
  return SUCCESS;
}

int Frontend::select_attrlist_from_join_where(char relname_source_one[ATTR_SIZE], char relname_source_two[ATTR_SIZE],
                                              char relname_target[ATTR_SIZE],
                                              char join_attr_one[ATTR_SIZE], char join_attr_two[ATTR_SIZE],
                                              int attr_count, char attr_list[][ATTR_SIZE]) {
  // Algebra::join + project
  return SUCCESS;
}

int Frontend::custom_function(int argc, char argv[][ATTR_SIZE]) {
  // argc gives the size of the argv array
  // argv stores every token delimited by space and comma

  // implement whatever you desire

  return SUCCESS;
}