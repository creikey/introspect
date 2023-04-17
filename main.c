#pragma warning(disable : 4996) // fopen is safe. I don't care about fopen_s

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <clang-c/Index.h>
#include <raylib.h>
#include <raymath.h>
#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>

#define ARRITER(type, a) for(int i = 0; i < arrlen(a); i++) for(type *it = &a[i]; it != 0; it = 0)

Font font = {0};

char ** identifiers = 0; // identifiers
#define IDENT_INVALID (-1)
typedef int IdentID; // index into identifiers

typedef struct
{
 int line;
 int column;
 int end_column;
 bool is_usage; // if not usage, is a declaration
 IdentID identifier;
} TextThing;

typedef struct
{
 const char *filename;
 char *text; // loaded by raylib
 size_t text_len;
 TextThing *decls_and_usages;
} File;

IdentID find_ident(const char *name)
{
 IdentID found_identifier = IDENT_INVALID;
 for(int i = 0; i < arrlen(identifiers); i++)
 {
  if(strcmp(identifiers[i], name) == 0)
  {
   found_identifier = (IdentID)i;
   break;
  }
 }

 return found_identifier;
}

char *into_arr(CXString s)
{
 const char *str = clang_getCString(s);
 assert(str);

 char *to_return = 0;
 
 size_t len = strlen(str);
 for(int i = 0; i < len; i++)
 {
  arrput(to_return, str[i]);
 }

 arrput(to_return, '\0');

 clang_disposeString(s);

 return to_return;
}

File *files = 0;

typedef struct NodeTextThing
{
 TextThing *pointing_at;
 Vector2 position;
 Vector2 vel;
 struct NodeTextThing *connected_to;
} NodeTextThing;

NodeTextThing *nodes = 0;


// And it’s so darn easy. Really. Those Clang folks really did an awesome work

File *to_fill = 0;
enum CXChildVisitResult on_visit(CXCursor cursor, CXCursor parent, CXClientData client_data)
{
 enum CXCursorKind kind = clang_getCursorKind(cursor);
 char *name_str = into_arr(clang_getCursorSpelling(cursor));
 char *kind_spelling_str = into_arr(clang_getCursorKindSpelling(kind));

 CXType cursor_type = clang_getCursorType(cursor);
 CXSourceLocation loc = clang_getCursorLocation(cursor);
 if(cursor_type.kind == CXType_Record)
 {
  CXCursor record_cursor = clang_getTypeDeclaration(cursor_type);
  arrfree(name_str);
  name_str = into_arr(clang_getCursorSpelling(record_cursor));
 }

 //if(cursor_type.kind == CXCursor_UnexposedExpr)
 CXTranslationUnit cursor_translation_unit = clang_Cursor_getTranslationUnit(cursor);
 CXToken *tok_ptr = clang_getToken(cursor_translation_unit, loc);
 if(tok_ptr)
 {
  arrfree(name_str);
  name_str = into_arr(clang_getTokenSpelling(cursor_translation_unit, *tok_ptr));
 }
 if(!clang_Location_isFromMainFile(loc))
 {
  return CXChildVisit_Recurse;
 }
 unsigned int line = 0;
 unsigned int column = 0;
 clang_getPresumedLocation(loc, 0, &line, &column);

 line -= 1;
 column -= 1;


 CXCursor semantic_parent = clang_getCursorSemanticParent(cursor);
 char *semantic_parent_kind_str = into_arr(clang_getCursorKindSpelling(clang_getCursorKind(semantic_parent)));

 char *parent_kind_str = into_arr(clang_getCursorKindSpelling(clang_getCursorKind(parent)));
 printf("kind `%s` | LINE %d column %d | Semantic parent kind `%s` | PARENT KIND `%s` | name `%s`\n", kind_spelling_str, line, column, semantic_parent_kind_str, parent_kind_str, name_str);
 arrfree(parent_kind_str);
 arrfree(semantic_parent_kind_str);

 bool is_reference_to_decl = kind == CXCursor_DeclRefExpr || kind == CXCursor_UnexposedExpr || kind == CXCursor_TypeRef;
 bool is_decl = kind == CXCursor_VarDecl || kind == CXCursor_FunctionDecl || kind == CXCursor_ParmDecl || kind == CXCursor_FieldDecl || kind == CXCursor_TypedefDecl;

 if(to_fill == 0)
 {
  // just filling out identifiers array
  if(is_decl)
  {
   char *new_identifier = 0;
   for(int i = 0; i < arrlen(name_str); i++)
   {
    arrput(new_identifier, name_str[i]);
   }

   bool duplicate = false;
   for(int i = 0; i < arrlen(identifiers); i++)
   {
    if(strcmp(identifiers[i], new_identifier) == 0)
    {
     duplicate = true;
    }
   }
   if(!duplicate)
   {
    arrput(identifiers, new_identifier);
   }
  }
 }
 else
 {
  if(is_reference_to_decl || is_decl)
  {
   IdentID found_identifier = find_ident(name_str);
   if(found_identifier == IDENT_INVALID)
   {
    //printf("Unknown identifier %s\n", name_str);
   }
   else
   {
    TextThing cur_thing = { .identifier = found_identifier, .line = line, .column = column, .end_column = column + (int)arrlen(name_str)-1 };
    if(is_decl)
    {
     cur_thing.is_usage = false;
    }
    else if(is_reference_to_decl)
    {
     cur_thing.is_usage = true;
    }
    else
    {
     assert(false);
    }

    bool duplicate = false;
    for(int i = 0; i < arrlen(to_fill->decls_and_usages); i++)
    {
     TextThing *a = &to_fill->decls_and_usages[i];
     TextThing *b = &cur_thing;
     bool at_same_spot = a->line == b->line && a->column == b->column;
     if(at_same_spot)
     {
      duplicate = true;
      break;
     }
    }
    if(duplicate)
    {
    }
    else
    {
     arrput(to_fill->decls_and_usages, cur_thing);
    }
   }
  }
 }

 arrfree(name_str);
 arrfree(kind_spelling_str);

 return CXChildVisit_Recurse;
}

int text_thing_cmp (const void * a, const void * b) {
 TextThing *a_thing = (TextThing*)a;
 TextThing *b_thing = (TextThing*)b;

 int result = a_thing->line - b_thing->line;

 if(result == 0)
 {
  result = a_thing->column - b_thing->column;
 }

 return result;
}

const float font_size = 32.0f;
const int max_cols = 50;
const int lines_above_and_below = 3;

Vector2 node_size()
{
 Vector2 glyph_size = MeasureTextEx(font, "A", font_size, 0.0f);
 return (Vector2){glyph_size.x*max_cols, (lines_above_and_below*2 + 1)*glyph_size.y};
}

Vector2 node_center(NodeTextThing *n)
{
 return Vector2Add(n->position, Vector2Scale(node_size(), 0.5f));
}

double clamp(double d, double min, double max) {
  const double t = d < min ? min : d;
  return t > max ? max : t;
}


int main(int argc, char **argv)
{
 if(argc <= 1)
 {
  printf("No C files to browse given.\n");
  return 0;
 }

 for(int i = 1; i < argc; i++)
 {
  CXIndex index = clang_createIndex(0, 0);
  printf("Parsing `%s`\n", argv[i]);
  CXTranslationUnit unit = clang_parseTranslationUnit(
    index,
    argv[i], NULL, 0,
    NULL, 0,
    CXTranslationUnit_None);

  assert(unit != 0);

  CXCursor cursor = clang_getTranslationUnitCursor(unit);
  to_fill = 0;
  clang_visitChildren(cursor, on_visit, NULL);

  clang_disposeTranslationUnit(unit);
  clang_disposeIndex(index);
 }

 for(int i = 0; i < arrlen(identifiers); i++)
 {
  printf("Found identifier %s\n", identifiers[i]);
 }

 for(int i = 1; i < argc; i++)
 {
  File new_file = {0};
  new_file.text = LoadFileText(argv[i]);
  new_file.text_len = strlen(new_file.text);
  new_file.filename = argv[i];

  CXIndex index = clang_createIndex(0, 0);
  CXTranslationUnit unit = clang_parseTranslationUnit(
    index,
    argv[i], NULL, 0,
    NULL, 0,
    CXTranslationUnit_None);
  assert(unit);

  CXCursor cursor = clang_getTranslationUnitCursor(unit);
  to_fill = &new_file;
  clang_visitChildren(cursor, on_visit, NULL);

  qsort(to_fill->decls_and_usages, arrlen(to_fill->decls_and_usages), sizeof(TextThing), text_thing_cmp);

  arrput(files, new_file);

  clang_disposeTranslationUnit(unit);
  clang_disposeIndex(index);
 }

 ARRITER(TextThing, files->decls_and_usages)
 {
  int random_max = 1000;
  Vector2 new_pos = (Vector2){(float)GetRandomValue(-random_max, random_max), (float)GetRandomValue(-random_max, random_max)};
  arrput(nodes, ((NodeTextThing){.pointing_at = it, .position = new_pos}) );
 }

 ARRITER(NodeTextThing, nodes)
 {
  NodeTextThing *from = it;
  from->connected_to = 0;
  if(from->pointing_at->is_usage)
  {
   ARRITER(NodeTextThing, nodes)
   {
    if(!it->pointing_at->is_usage && it->pointing_at->identifier == from->pointing_at->identifier)
    {
     from->connected_to = it;
     break;
    }
   }
  }
 }

 const int screenWidth = 800;
 const int screenHeight = 450;

 SetConfigFlags(FLAG_WINDOW_RESIZABLE);
 InitWindow(screenWidth, screenHeight, "Introspect");

 font = LoadFont("RobotoMono-Regular.ttf");

 SetTargetFPS(60);
 Camera2D camera = { 0 };

 camera.zoom = 1.0f;

 while (!WindowShouldClose())
 {
  Vector2 scaled_delta = Vector2Scale(GetMouseDelta(), 1.0f/camera.zoom);
  if(IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
  {
   camera.target = Vector2Subtract(camera.target, scaled_delta);
  }
  
  float zoom_input = (float)GetMouseWheelMoveV().y*0.1f;

  // I'm deeply unserious I'm deeply unserious
  // I'm deeply unserious I'm deeply unserious

  
  /*
   * 
   */

  Vector2 want_mouse_pos = GetMousePosition();
  Vector2 before_world_space = GetScreenToWorld2D(want_mouse_pos, camera);

#if 1
  camera.zoom *= (1.0f + zoom_input);
  camera.zoom = (float)clamp(camera.zoom, 0.01f, 100.0);
#else
  camera.zoom *= powf(2.0f, zoom_input);
#endif

  Vector2 mouse_delta = Vector2Subtract(GetWorldToScreen2D(before_world_space, camera), want_mouse_pos);
  mouse_delta = Vector2Scale(mouse_delta, 1.0f/camera.zoom);
  camera.target = Vector2Add(camera.target, mouse_delta);

  /*
  */

  BeginDrawing();
  BeginMode2D(camera);

  ClearBackground(RAYWHITE);

  typedef struct
  {
   char c;
   Color col;
  } DrawnCharacter;


  Vector2 glyph_size = MeasureTextEx(font, "A", font_size, 0.0f);

  Vector2 node_code_size = node_size();
   
  static NodeTextThing *dragging = 0;
  if(dragging && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) dragging = 0;

  File *to_draw = &files[0];
  Vector2 world_mouse = GetScreenToWorld2D(GetMousePosition(), camera);
  float dt = GetFrameTime();
  ARRITER(NodeTextThing, nodes)
  {
   NodeTextThing *from = it;
   ARRITER(NodeTextThing, nodes)
   {
    Vector2 from_center = node_center(from);
    Vector2 to_center = node_center(it);

    Vector2 towards = Vector2Subtract(to_center, from_center);
    if(Vector2Length(towards) < node_code_size.x/2.0f)
    {
     it->vel = Vector2Add(it->vel, Vector2Scale(towards, dt*10.0f));
    }
   }

   it->vel = Vector2Scale(it->vel, powf(0.01f, dt));

   it->position = Vector2Add(it->position, Vector2Scale(it->vel, dt));
   bool hovering = CheckCollisionPointRec(world_mouse, (Rectangle){
    .x = it->position.x,
    .y = it->position.y,
    .width = node_code_size.x,
    .height = node_code_size.y,
   });
    
   unsigned char alpha = 30;
   if(hovering) alpha = 15;
   bool want_dragging = hovering && IsMouseButtonDown(MOUSE_BUTTON_LEFT);

   if(want_dragging && dragging == 0) dragging = it;

   if(it == dragging)
   {
    alpha = 45;
    it->position = Vector2Add(it->position, scaled_delta);
   }
    
   DrawRectangleV(it->position, node_code_size, (Color){.a = alpha} );
   DrawnCharacter *cur_line = 0;

   if(it->connected_to)
   {
    Vector2 from = node_center(it);
    Vector2 to = node_center(it->connected_to);
    DrawLineV(from, to, BLACK);

    Vector2 towards = Vector2Subtract(to, from);
    it->vel = Vector2Add(it->vel, Vector2Scale(towards, dt*10.0f));
   }

   TextThing *to_highlight = it->pointing_at;
   int line_from = to_highlight->line - lines_above_and_below;
   int line_to = to_highlight->line + lines_above_and_below;

   int cur_line_index = 0;
   int cur_col_index = 0;
   int cursor = 0;
   float vertical_position = it->position.y;

   // skip to the line
   while(cur_line_index < line_from)
   {
    if(cursor >= to_draw->text_len) break;

    char curchar = to_draw->text[cursor];
    if(curchar == '\n')
    {
     cur_line_index++;
    }

    cursor++;
   }

   while(cursor < to_draw->text_len)
   {
    if(cur_line_index >= line_to) break;

    char curchar = to_draw->text[cursor];
    if(curchar == '\r')
    {
    }
    else if(curchar == '\n' || cursor == to_draw->text_len - 1 || cur_col_index >= max_cols)
    {
     float vertical_size = glyph_size.y;
     float horizontal_position = it->position.x;
     for(int i = 0; i < arrlen(cur_line); i++)
     {
      char cur_char_as_str[2] = {0};
      cur_char_as_str[0] = cur_line[i].c;
      DrawTextEx(font, cur_char_as_str, (Vector2){horizontal_position,vertical_position}, font_size, 0.0f, cur_line[i].col);
      Vector2 drawn_size = MeasureTextEx(font, cur_char_as_str, font_size, 0.0f);
      horizontal_position += drawn_size.x;
      vertical_size = drawn_size.y;
     }
     const char *line_number_str = TextFormat("%d", cur_line_index);
     float line_number_width = MeasureTextEx(font, line_number_str, font_size, 0.0f).x;
     DrawTextEx(font, line_number_str, (Vector2){it->position.x - line_number_width, vertical_position}, font_size, 0.0f, LIGHTGRAY);

     arrfree(cur_line);
     cur_line = 0;
     vertical_position += vertical_size;

     if(curchar == '\n')
     {
      cur_line_index += 1;
     }
     cur_col_index = -1; // will be incremented in the next line
    }
    else
    {
     DrawnCharacter new_drawn = { .c = to_draw->text[cursor], .col = BLACK };

     {
      if(to_highlight->line == cur_line_index && to_highlight->column <= cur_col_index && cur_col_index < to_highlight->end_column)
      {
       if(to_highlight->is_usage)
       {
        new_drawn.col = BLUE;
       }
       else
       {
        new_drawn.col = ORANGE;
       }
      }
     }
     arrput(cur_line, new_drawn);
    }

    cur_col_index += 1;
    cursor += 1;
   }
   assert(cur_line == 0);
  }

  EndMode2D();
  EndDrawing();
 }

 CloseWindow();

 ARRITER(File, files)
 {
  arrfree(it->decls_and_usages);
  UnloadFileText(it->text);
 }

 ARRITER(char*, identifiers)
 {
  arrfree(*it);
 }
 arrfree(identifiers);
 arrfree(nodes);
}
