/*
 * Copyright (C) 2018 Intel Corporation.All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file main.cpp
 * @date 2017
 * @brief Test for SmartXDebugFascadeTest
 */

#include "SmartXDebugFacadeTest.hpp"

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/parser.h>

static int new_node_count = 0;
static int add_child_count = 0;
int __xmlRegisterCallbacks = 0;

/* libxml2 mock xmlNewNode function to fail the xmlnewnode return */
xmlNodePtr xmlNewNode(xmlNsPtr ns, const xmlChar *name)
{
  (void)ns;
  (void)name;

  if (new_node_count == 0 || new_node_count == 2 || new_node_count == 4)
  {
    new_node_count++;
    return nullptr;
  }
  else //actual function source taken from libxml2 library
  {
    new_node_count++;
    xmlNodePtr cur;

    if (name == NULL)
    {
#ifdef DEBUG_TREE
      xmlGenericError(xmlGenericErrorContext,
          "xmlNewNode : name == NULL\n");
#endif
      return(NULL);
    }

    /*
     * Allocate a new node and fill the fields.
     */
    cur = (xmlNodePtr) xmlMalloc(sizeof(xmlNode));
    if (cur == NULL)
    {
      //xmlTreeErrMemory("building node");
      return(NULL);
    }
    memset(cur, 0, sizeof(xmlNode));
    cur->type = XML_ELEMENT_NODE;

    cur->name = xmlStrdup(name);
    cur->ns = ns;

    if ((__xmlRegisterCallbacks) && (xmlRegisterNodeDefaultValue))
    {
      xmlRegisterNodeDefaultValue(cur);
    }
    return(cur);
  }
}

/* libxml2 mock xmlAddChild function to fail the return */
xmlNodePtr xmlAddChild(xmlNodePtr parent, xmlNodePtr cur)
{
  if (add_child_count == 0)
  {
    (void)parent;
    (void)cur;
    add_child_count++;
    return nullptr;
  }
  else
  {
    add_child_count++;
    xmlNodePtr prev;

    if (parent == NULL)
    {
#ifdef DEBUG_TREE
      xmlGenericError(xmlGenericErrorContext,
          "xmlAddChild : parent == NULL\n");
#endif
      return(NULL);
    }

    if (cur == NULL) {
#ifdef DEBUG_TREE
      xmlGenericError(xmlGenericErrorContext,
          "xmlAddChild : child == NULL\n");
#endif
      return(NULL);
    }

    if (parent == cur)
    {
#ifdef DEBUG_TREE
      xmlGenericError(xmlGenericErrorContext,
          "xmlAddChild : parent == cur\n");
#endif
      return(NULL);
    }
    /*
     * If cur is a TEXT node, merge its content with adjacent TEXT nodes
     * cur is then freed.
     */
    if (cur->type == XML_TEXT_NODE)
    {
      if ((parent->type == XML_TEXT_NODE) &&
          (parent->content != NULL) &&
          (parent->name == cur->name))
      {
        xmlNodeAddContent(parent, cur->content);
        xmlFreeNode(cur);
        return(parent);
      }
      if ((parent->last != NULL) && (parent->last->type == XML_TEXT_NODE) &&
          (parent->last->name == cur->name) &&
          (parent->last != cur))
      {
        xmlNodeAddContent(parent->last, cur->content);
        xmlFreeNode(cur);
        return(parent->last);
      }
    }

    /*
     * add the new element at the end of the children list.
     */
    prev = cur->parent;
    cur->parent = parent;
    if (cur->doc != parent->doc)
    {
      xmlSetTreeDoc(cur, parent->doc);
    }
    /* this check prevents a loop on tree-traversions if a developer
     * tries to add a node to its parent multiple times
     */
    if (prev == parent)
    {
      return(cur);
    }

    /*
     * Coalescing
     */
    if ((parent->type == XML_TEXT_NODE) &&
        (parent->content != NULL) &&
        (parent != cur))
    {
      xmlNodeAddContent(parent, cur->content);
      xmlFreeNode(cur);
      return(parent);
    }
    if (cur->type == XML_ATTRIBUTE_NODE)
    {
      if (parent->type != XML_ELEMENT_NODE)
        return(NULL);
      if (parent->properties != NULL)
      {
        /* check if an attribute with the same name exists */
        xmlAttrPtr lastattr;

        if (cur->ns == NULL)
        {
          lastattr = xmlHasNsProp(parent, cur->name, NULL);
        }
        else
        {
          lastattr = xmlHasNsProp(parent, cur->name, cur->ns->href);
        }
        if ((lastattr != NULL) && (lastattr != (xmlAttrPtr) cur) && (lastattr->type != XML_ATTRIBUTE_DECL))
        {
          /* different instance, destroy it (attributes must be unique) */
          xmlUnlinkNode((xmlNodePtr) lastattr);
          xmlFreeProp(lastattr);
        }
        if (lastattr == (xmlAttrPtr) cur)
        {
          return(cur);
        }
      }
      if (parent->properties == NULL)
      {
        parent->properties = (xmlAttrPtr) cur;
      } else
      {
        /* find the end */
        xmlAttrPtr lastattr = parent->properties;
        while (lastattr->next != NULL)
        {
          lastattr = lastattr->next;
        }
        lastattr->next = (xmlAttrPtr) cur;
        ((xmlAttrPtr) cur)->prev = lastattr;
      }
    }
    else
    {
      if (parent->children == NULL)
      {
        parent->children = cur;
        parent->last = cur;
      }
      else
      {
        prev = parent->last;
        prev->next = cur;
        cur->prev = prev;
        parent->last = cur;
      }
    }
    return(cur);
  }
}


Ias::Int32 main(Ias::Int32 argc, Ias::Char **argv)
{
  setenv("DLT_INITIAL_LOG_LEVEL", "::6", true);
  (void)argc;
  (void)argv;
  IasAudio::IasAudioLogging::registerDltApp(true);

  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();
  return result;
}

